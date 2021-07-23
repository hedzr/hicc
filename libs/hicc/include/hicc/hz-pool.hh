//
// Created by Hedzr Yeh on 2021/7/13.
//

#ifndef HICC_CXX_POOL_HH
#define HICC_CXX_POOL_HH

#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>

#include <functional>
#include <optional>
#include <queue>
#include <random>
#include <string>
#include <vector>

#include "hz-defs.hh"
#include "hz-ringbuf.hh"

namespace hicc::pool {

    template<typename Pred = std::function<bool()>, typename Setter = std::function<void()>>
    class conditional_wait {
        std::condition_variable cv;
        std::mutex m;
        Pred p;
        Setter s;

    public:
        conditional_wait(Pred &&p_, Setter &&s_)
            : p(std::move(p_))
            , s(std::move(s_)) {}
        ~conditional_wait() {}
        conditional_wait(conditional_wait &&) = delete;
        conditional_wait &operator=(conditional_wait &&) = delete;

    public:
        void wait() {
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk, p);
        }
        void set() {
            {
                std::unique_lock<std::mutex> lk(m);
                s();
            }
            cv.notify_one();
        }
        void set_for_all() {
            {
                std::unique_lock<std::mutex> lk(m);
                s();
            }
            cv.notify_all();
        }
    };

    class conditional_wait_for_bool : public conditional_wait<> {
        bool var;

    public:
        conditional_wait_for_bool()
            : conditional_wait([this]() { return _wait(); }, [this]() { _set(); }) {}
        ~conditional_wait_for_bool() { release(); }
        conditional_wait_for_bool(conditional_wait_for_bool &&) = delete;
        conditional_wait_for_bool &operator=(conditional_wait_for_bool &&) = delete;

    private:
        bool _wait() const { return var; }
        void _set() { var = true; }
        void release() {
            //
        }
    };

    class conditional_wait_for_int : public conditional_wait<> {
        int var;
        int max_value;

    public:
        conditional_wait_for_int(int max_value_ = 1)
            : conditional_wait([this]() { return _wait(); }, [this]() { _set(); })
            , var(0)
            , max_value(max_value_) {}
        ~conditional_wait_for_int() { release(); }
        conditional_wait_for_int(conditional_wait_for_int &&) = delete;
        conditional_wait_for_int &operator=(conditional_wait_for_int &&) = delete;

    private:
        inline bool _wait() const { return var >= max_value; }
        inline void _set() { var++; }
        inline void release() {
            //
        }
    };


    template<class T>
    class threaded_message_queue {
    public:
        using lock = std::unique_lock<std::mutex>;
        void emplace_back(T &&t) {
            {
                lock l(_m);
                _data.emplace_back(std::move(t));
            }
            _cv.notify_one();
        }
        void push_back(T const &t) {
            {
                lock l(_m);
                _data.push_back(t);
            }
            _cv.notify_one();
        }
        // void push_back_bad(T t) {
        //     {
        //         lock l(_m);
        //         _data.push_back(std::move(t));
        //     }
        //     _cv.notify_one();
        // }

        inline std::optional<T> pop_front() {
            std::optional<T> ret;
            lock l(_m);
            _cv.wait(l, [this] { return _abort || !_data.empty(); });
            if (_abort) return ret; // std::nullopt;

            ret.emplace(std::move(_data.back()));
            _data.pop_back();
            return ret;
        }
        // inline std::optional<T> pop_front_bad() {
        //     lock l(_m);
        //     _cv.wait(l, [this] { return _abort || !_data.empty(); });
        //     if (_abort) return {}; // std::nullopt;
        //
        //     auto r = std::move(_data.back());
        //     _data.pop_back();
        //     return r;
        // }

        void clear() {
            {
                lock l(_m);
                _abort = true;
                _data.clear();
            }
            _cv.notify_all();
        }
        ~threaded_message_queue() { clear(); }

    private:
        std::mutex _m;
        std::deque<T> _data;
        std::condition_variable _cv;
        bool _abort;
    }; // class threaded_message_queue

    /**
     * @brief a c++11 thread pool with pre-created, fixed running threads and free tasks management.
     * 
     * Each thread will try to lock the task queue and fetch the newest one for launching.
     * 
     * This pool was inspired by someone but I have no idea to find out original post. so anyone
     * if you know could issue me.
     */
    class thread_pool {
    public:
        thread_pool(std::size_t n = 1u)
            : _cv_started((int)n) { start_thread(n); }
        thread_pool(thread_pool &&) = delete;
        thread_pool &operator=(thread_pool &&) = delete;
        ~thread_pool() { join(); }

    public:
        template<class F, class R = std::result_of_t<F &()>>
        std::future<R> queue_task(F &&task) {
            std::packaged_task<R()> p(std::move(task));
            auto r = p.get_future();
            // _tasks.push_back(std::move(p));
            _tasks.emplace_back(std::move(p));
            return r;
        }
        void join() { clear_threads(); }
        std::size_t active_threads() const { return _active; }
        std::size_t total_threads() const { return _threads.size(); }
        auto &tasks() { return _tasks; }
        auto const &tasks() const { return _tasks; }

    private:
        template<class F, class R = std::result_of_t<F &()>>
        std::future<R> run_task(F &&task) {
            if (active_threads() >= total_threads()) {
                start_thread();
            }
            return queue_task(std::move(task));
        }
        void clear_threads() {
            _tasks.clear();
            _threads.clear();
        }
        void start_thread(std::size_t n = 1) {
            while (n-- > 0) {
                _threads.push_back(
                        std::async(std::launch::async,
                                   [this] {
                                       _cv_started.set();
                                       while (auto task = _tasks.pop_front()) {
                                           ++_active;
                                           try {
                                               (*task)();
                                           } catch (...) {
                                               --_active;
                                               throw;
                                           }
                                           --_active;
                                       }
                                   }));
            }

            _cv_started.wait();
        }

    private:
        std::vector<std::future<void>> _threads;                   // fixed, running pool
        threaded_message_queue<std::packaged_task<void()>> _tasks; // the futures
        std::atomic<std::size_t> _active;
        conditional_wait_for_int _cv_started;
    }; // class thread_pool

} // namespace hicc::pool

#endif //HICC_CXX_POOL_HH
