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

    template<class T>
    class threaded_message_queue {
    public:
        using lock = std::unique_lock<std::mutex>;
        void emplace_back(T &&t) {
            {
                lock l(_m);
                _data.template emplace_back(std::move(t));
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

            ret.template emplace(std::move(_data.back()));
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
        ~threaded_message_queue() {
            clear();
        }

    private:
        std::mutex _m;
        std::deque<T> _data;
        std::condition_variable _cv;
        bool _abort = false;
    }; // class threaded_message_queue

    class thread_pool {
    public:
        thread_pool(std::size_t n = 1) { start_thread(n); }
        thread_pool(thread_pool &&) = delete;
        thread_pool &operator=(thread_pool &&) = delete;
        ~thread_pool() { join(); }
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
        std::future<R> run_task(F task) {
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
        }

    private:
        std::vector<std::future<void>> _threads;                   // fixed, running pool
        threaded_message_queue<std::packaged_task<void()>> _tasks; // the futures
        std::atomic<std::size_t> _active;
    }; // class thread_pool

} // namespace hicc::pool

#endif //HICC_CXX_POOL_HH
