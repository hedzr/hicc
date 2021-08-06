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

    template<typename T, typename Pred = std::function<bool()>, typename Setter = std::function<void()>>
    class conditional_wait {
        std::condition_variable cv;
        std::mutex m;
        Pred p;
        Setter s;

    protected:
        T _var;

    public:
        explicit conditional_wait(Pred &&p_, Setter &&s_)
            : p(std::move(p_))
            , s(std::move(s_)) {}
        virtual ~conditional_wait() { clear(); }
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
        void clear() { _release(); }
        T const &val() const { return _value(); }
        T &val() { return _value(); }

    protected:
        virtual T const &_value() const { return _var; }
        virtual T &_value() { return _var; }
        virtual void _release() {}
    };

    class conditional_wait_for_bool : public conditional_wait<bool> {
    public:
        conditional_wait_for_bool()
            : conditional_wait([this]() { return _wait(); }, [this]() { _set(); }) {}
        virtual ~conditional_wait_for_bool() = default;
        conditional_wait_for_bool(conditional_wait_for_bool &&) = delete;
        conditional_wait_for_bool &operator=(conditional_wait_for_bool &&) = delete;

    protected:
        bool _wait() const { return _var; }
        void _set() { _var = true; }
    };

    class conditional_wait_for_int : public conditional_wait<int> {
    public:
        conditional_wait_for_int(int max_value_ = 1)
            : conditional_wait([this]() { return _wait(); }, [this]() { _set(); })
            , _max_value(max_value_) {}
        virtual ~conditional_wait_for_int() = default;
        conditional_wait_for_int(conditional_wait_for_int &&) = delete;
        conditional_wait_for_int &operator=(conditional_wait_for_int &&) = delete;

        inline int max_val() const { return _max_value; }

    protected:
        inline bool _wait() const { return _var >= _max_value; }
        inline void _set() { _var++; }

    private:
        int _max_value;
    };


    /**
     * @brief helper class for shutdown a sleep loop gracefully.
     * 
     * @details Sample:
     * @code{c++}
     * class X {
     *     std::thread _t;
     *     hicc::pool::timer_killer _tk;
     *     static void runner(timer *_this) {
     *         using namespace std::literals::chrono_literals;
     *         auto d = 10ns;
     *         while (_this->_tk.wait_for(d)) {
     *             // std::this_thread::sleep_for(d);
     *             std::this_thread::yield();
     *         }
     *         hicc_trace("timer::runner ended.");
     *     }
     *     void start(){ _t.detach(); }
     *   public:
     *     X(): _t(std::thread(runner, this)) { start(); }
     *     ~X(){ stop(); }
     *     void stop() {  _tk.kill(); }
     * };
     * @endcode
     */
    class timer_killer {
        bool _terminate = false;
        mutable std::condition_variable _cv;
        mutable std::mutex _m;

        timer_killer(timer_killer &&) = delete;
        timer_killer(timer_killer const &) = delete;
        timer_killer &operator=(timer_killer &&) = delete;
        timer_killer &operator=(timer_killer const &) = delete;

    public:
        timer_killer() = default;
        // returns false if killed:
        template<class R, class P>
        bool wait_for(std::chrono::duration<R, P> const &time) const {
            std::unique_lock<std::mutex> lock(_m);
            return !_cv.wait_for(lock, time, [&] { return _terminate; });
        }
        bool terminated() const {
            std::unique_lock<std::mutex> lock(_m);
            return _terminate;
        }
        void kill() {
            bool go{false};
            {
                std::unique_lock<std::mutex> lock(_m);
                if (!_terminate) {
                    go = _terminate = true; // should be modified inside mutex lock
                }
            }
            if (go)
                _cv.notify_all(); // it is safe, and *sometimes* optimal, to do this outside the lock}
        }
    };


    template<class T>
    class threaded_message_queue {
    public:
        using lock = std::unique_lock<std::mutex>;
        void emplace_back(T &&t) {
            {
                lock l_(_m);
                _data.emplace_back(std::move(t));
            }
            _cv.notify_one();
        }
        // void push_back(T const &t) {
        //     {
        //         lock l_(_m);
        //         _data.push_back(t);
        //     }
        //     _cv.notify_one();
        // }
        // void push_back_bad(T t) {
        //     {
        //         lock l(_m);
        //         _data.push_back(std::move(t));
        //     }
        //     _cv.notify_one();
        // }

        inline std::optional<T> pop_front() {
            std::optional<T> ret;
            {
                lock l_(_m);
                _cv.wait(l_, [this] { return _abort || !_data.empty(); });
                if (_abort) {
                    return ret; // std::nullopt;
                }
                ret.emplace(std::move(_data.back()));
                _data.pop_back();
            }
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
                lock l_(_m);
                _abort = true;
                _data.clear();
            }
            _cv.notify_all();
        }
        ~threaded_message_queue() { clear(); }

        bool empty() const { return _data.empty(); }

    private:
        std::mutex _m;
        std::condition_variable _cv;
        bool _abort;
        mutable std::deque<T> _data;
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
        thread_pool(int n = 1u)
            : _cv_started((int) (n > 0 ? n : std::thread::hardware_concurrency())) { start_thread((n > 0 ? n : std::thread::hardware_concurrency())); }
        thread_pool(thread_pool &&) = delete;
        thread_pool &operator=(thread_pool &&) = delete;
        ~thread_pool() { join(); }

    public:
        template<class F, class R = std::result_of_t<F &()>>
        std::future<R> queue_task(F &&task) {
            auto p = std::packaged_task<R()>(std::forward<F>(task));
            // std::packaged_task<R()> p(std::move(task));
            auto r = p.get_future();
            // _tasks.push_back(std::move(p));
            _tasks.emplace_back(std::move(p));
            return r;
        }
        template<class F, class R = std::result_of_t<F &()>>
        std::future<R> queue_task(F const &task) {
            auto p = std::packaged_task<R()>(std::forward<F>(task));
            // std::packaged_task<R()> p(std::move(task));
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
        std::vector<std::future<void>> _threads; // fixed, running pool

        std::atomic<std::size_t> _active;
        conditional_wait_for_int _cv_started;

        mutable threaded_message_queue<std::packaged_task<void()>> _tasks; // the futures
    };                                                                     // class thread_pool

} // namespace hicc::pool

#endif //HICC_CXX_POOL_HH
