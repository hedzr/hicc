//
// Created by Hedzr Yeh on 2021/7/13.
//

#ifndef HICC_CXX_TICKER_HH
#define HICC_CXX_TICKER_HH

#include <chrono>
#include <ctime>

#include <iomanip>
#include <iostream>
#include <map>
#include <queue>         // for std::priority_queue
#include <string>        // for std::string
#include <tuple>         // for std::tuple
#include <unordered_map> // for std::unordered_map
#include <vector>

#include <algorithm>
#include <functional>

#include "hz-defs.hh"
#include "hz-log.hh"
#include "hz-pool.hh"

#include "hz-chrono.hh"


namespace hicc::chrono {

    using Clock = std::chrono::system_clock;

    class timer_job {
    public:
        explicit timer_job(std::function<void()> &&f, bool recur = false, bool interval = false)
            : _recur(recur)
            , _interval(interval)
            , _f(std::move(f))
            , _hit(0) {}
        virtual ~timer_job() {}
        virtual Clock::time_point next_time_point() const = 0;

        void launch_to(pool::thread_pool &p, std::function<void(timer_job &tj)> const &post_job = nullptr) {
            launch_fn_to_pool(_f, p, post_job);
        }

    private:
        void launch_fn_to_pool(std::function<void()> const &fn, pool::thread_pool &pool, std::function<void(timer_job &tj)> const &post_job = nullptr) {
            pool.queue_task([fn, post_job, this]() {
                fn();
                if (post_job)
                    post_job(*this);
            });
#if defined(_DEBUG) || HICC_TEST_THREAD_POOL_DBGOUT
            if ((_hit % 10) == 0)
                pool_debug("job launched, %p (_recur=%d, _interval=%d, hit=%u)", (void *) this, _recur, _interval, _hit);
#endif
            ++_hit;
            std::this_thread::yield();
        }

    public:
        bool _recur;
        bool _interval;

    protected:
        std::function<void()> _f;
        std::size_t _hit;
    };

    namespace detail {
        class in_job : public timer_job {
        public:
            explicit in_job(std::function<void()> &&f)
                : timer_job(std::move(f)) {}

            // dummy time_point because it's not used
            Clock::time_point next_time_point() const override { return Clock::time_point(Clock::duration(0)); }
        };

        class every_job : public timer_job {
        public:
            every_job(Clock::duration d, std::function<void()> &&f, bool interval = false)
                : timer_job(std::move(f), true, interval)
                , dur(d) {}

            Clock::time_point next_time_point() const override {
#if defined(_DEBUG) || HICC_TEST_THREAD_POOL_DBGOUT
                auto now = Clock::now();
                auto nxt = now + dur;
                pool_debug("         %s -> %s", chrono::format_time_point(now).c_str(), chrono::format_time_point(nxt).c_str());
                return nxt;
#else
                return Clock::now() + dur;
#endif
            };

            Clock::duration dur;
        };
    } // namespace detail

    /**
     * @brief timer provides the standard Timer interface.
     * @tparam Clock 
     * @details We assume a standard Timer interface will be represented as:
     * 
     * ### A
     *     - in/afetr
     *     - at
     * 
     * 
     */
    template<typename Clock = Clock>
    class timer {
        // public:
        //     class posix_ticker {
        //     public:
        //     }; // class posix_ticker

    public:
        using Job = timer_job;
        using _J = std::shared_ptr<Job>;
        using _C = Clock;
        using TP = std::chrono::time_point<_C>;
        using Jobs = std::vector<_J>;
        using TimingWheel = std::map<TP, Jobs>;

    public:
        timer()
            : _pool(-1) { start(); }
        virtual ~timer() { clear(); }
        CLAZZ_NON_COPYABLE(timer);
        void clear() { stop(); }

        // template<class _Rep, class _Period>
        // timer &in(const std::chrono::duration<_Rep, _Period> &__d, std::function<void()> &&f) {
        //     UNUSED(__d, f);
        //     return (*this);
        // }

        /**
         * @brief run task in (one minute, five seconds, ...)
         * @tparam _Callable 
         * @tparam _Args 
         * @param time 
         * @param f 
         * @param args 
         * @return 
         */
        template<typename _Callable, typename... _Args>
        timer &in(const typename Clock::time_point time, _Callable &&f, _Args &&...args) {
            UNUSED(time, f);
            std::shared_ptr<Job> t = std::make_shared<detail::in_job>(
                    std::bind(std::forward<_Callable>(f), std::forward<_Args>(args)...));
            add_task(time, std::move(t));
            return (*this);
        }
        template<typename _Callable, typename... _Args>
        timer &in(const typename Clock::duration time, _Callable &&f, _Args &&...args) {
            in(Clock::now() + time, std::forward<_Callable>(f), std::forward<_Args>(args)...);
            return (*this);
        }
        template<typename _Callable, typename... _Args>
        timer &after(const typename Clock::time_point time, _Callable &&f, _Args &&...args) { return in(time, f, args...); }
        template<typename _Callable, typename... _Args>
        timer &after(const typename Clock::duration time, _Callable &&f, _Args &&...args) { return in(time, f, args...); }
        template<typename _Callable, typename... _Args>
        timer &at(const std::string &time, _Callable &&f, _Args &&...args) {
            UNUSED(time, f);

            auto time_now = Clock::to_time_t(Clock::now());
            std::tm tm = *std::localtime(&time_now);

            // our final time as a time_point
            typename Clock::time_point tp;

            if (chrono::try_parse(tm, time, "%H:%M:%S")) {
                // convert tm back to time_t, then to a time_point and assign to final
                tp = Clock::from_time_t(std::mktime(&tm));

                // if we've already passed this time, the user will mean next day, so add a day.
                if (Clock::now() >= tp)
                    tp += std::chrono::hours(24);
            } else if (try_parse(tm, time, "%Y-%m-%d %H:%M:%S")) {
                tp = Clock::from_time_t(std::mktime(&tm));
            } else if (try_parse(tm, time, "%Y/%m/%d %H:%M:%S")) {
                tp = Clock::from_time_t(std::mktime(&tm));
            } else {
                // could not parse time
                throw std::runtime_error("Cannot parse time string: " + time);
            }

            in(tp, std::forward<_Callable>(f), std::forward<_Args>(args)...);
            return (*this);
        }

    private:
        void stop() {
            _tk.kill();
            _ended.wait();
        }
        void start() {
            auto t = std::thread(runner, this);
            _started.wait();
            t.detach();
        }

        std::tuple<typename TimingWheel::iterator, typename TimingWheel::iterator, bool> find_next() {
            auto time_now = Clock::to_time_t(Clock::now());
            bool found{};

            std::unique_lock<std::mutex> l(_l_twl);
            typename TimingWheel::iterator itn = _twl.end();
            typename TimingWheel::iterator itp = itn;

            for (auto it = _twl.begin(); it != itn; ++it) {
                auto &tp = (*it).first;
                auto tmp = Clock::to_time_t(tp);
                if (tmp > time_now) break;
                itp = it;
                continue;
            }

            if (itp != itn) {
                found = true;
                itn = itp;
                itn++;
            }
            return {itp, itn, found};
        }

        static void runner(timer *_this) {
            hicc_trace("[runner] ready...");
            _this->runner_loop();
        }
        void runner_loop() {
            using namespace std::literals::chrono_literals;
            const auto starting_gap = 10ns;
            std::chrono::nanoseconds d = starting_gap;
#if defined(_DEBUG) || HICC_TEST_THREAD_POOL_DBGOUT
            std::size_t hit{0}, loop{0};
#endif
            _started.set();
            // std::cout << d;
            while (_tk.wait_for(d)) {
                // std::this_thread::sleep_for(d);
                // pool_debug("[runner] waked up");
                std::this_thread::yield();
                d = _larger_gap;

                TP picked;
                Jobs jobs, recurred_jobs;
                {
                    auto [itp, itn, found] = find_next();
                    if (found) {
                        // got a picked point
                        std::unique_lock<std::mutex> l(_l_twl);
                        (*itp).second.swap(jobs);
                        picked = (*itp).first;

                        _twl.erase(_twl.begin(), itp);

                        if (itn != _twl.end()) {
                            auto next_tp = (*itn).first;
                            d = (next_tp - picked);
                            if (d > _wastage)
                                d -= _wastage;
#if defined(_DEBUG) || HICC_TEST_THREAD_POOL_DBGOUT
                            if ((hit % 10) == 0)
                                pool_debug("[runner] [size: %u, hit: %u, loop: %u] picked = %s, next_tp = %s, duration = %s",
                                           _twl.size(), hit, loop,
                                           chrono::format_time_point(picked).c_str(),
                                           chrono::format_time_point(next_tp).c_str(),
                                           chrono::format_duration(d).c_str());
                            hit++;
#endif
                        }
                    }
                }

                // launch the jobs
                for (auto it = jobs.begin(); it != jobs.end(); ++it) {
                    auto &j = (*it);
                    if (j->_interval) {
                        // hicc_debug("[runner] job starting, _interval");
                        j->launch_to(_pool, [&j, this](timer_job &tj) {
                            add_task(tj.next_time_point(), std::move(j));
                        });
                    } else if (j->_recur) {
                        // hicc_debug("[runner] job starting, _recur");
                        j->launch_to(_pool);
                        recurred_jobs.emplace_back(std::move(j));
                    } else {
                        // hicc_debug("[runner] job starting");
                        j->launch_to(_pool);
                    }
                }

                for (auto &j : recurred_jobs) {
                    auto tp = j->next_time_point();
#if defined(_DEBUG) || HICC_TEST_THREAD_POOL_DBGOUT
                    auto dur = ((detail::every_job *) j.get())->dur;
                    auto size = add_task(tp, std::move(j));
                    if ((loop % 10) == 0)
                        pool_debug("[runner] [size: %u, hit: %u, loop: %u] _recur job/%d added: %s, dur = %s, d = %s",
                                   size, hit, loop, recurred_jobs.size(),
                                   chrono::format_time_point(tp).c_str(),
                                   chrono::format_duration(dur).c_str(),
                                   chrono::format_duration(d).c_str());
                    UNUSED(size, dur);
                    loop++;
#else
                    add_task(tp, std::move(j));
#endif
                    d = tp - now();
                    if (d > _wastage)
                        d -= _wastage;
                }

                std::this_thread::yield();
            }
            hicc_debug("[runner] timer::runner ended (%d).", _tk.terminated());
            _ended.set();
        }

    protected:
        std::size_t add_task(TP const &tp, std::shared_ptr<Job> &&task) {
            std::size_t size;
            {
                std::unique_lock<std::mutex> l(_l_twl);
                auto it = _twl.find(tp);
                if (it == _twl.end()) {
                    Jobs coll;
                    coll.emplace_back(std::move(task));
                    _twl.emplace(tp, std::move(coll));
                    pool_debug("add_task, tp not found. pool.size=%lu", _twl.size());
                } else {
                    auto &coll = (*it).second;
                    coll.emplace_back(std::move(task));
                    pool_debug("add_task, tp found. pool.size=%lu", _twl.size());
                }
                size = _twl.size();
            }
            std::this_thread::yield();
            return size;
        }
        std::size_t remove_task(TP const &tp, std::shared_ptr<Job> const &task) {
            std::size_t size;
            {
                std::unique_lock<std::mutex> l(_l_twl);

                auto it = _twl.find(tp);
                if (it != _twl.end()) {
                    auto &coll = (*it).second;
                    std::remove(coll.begin(), coll.end(), task);
                }
                size = _twl.size();
            }
            std::this_thread::yield();
            return size;
        }

    private:
        // std::thread _t;
        pool::timer_killer _tk{}; // to shut down the sleep+loop in `runner` thread gracefully
        TimingWheel _twl{};
        std::mutex _l_twl{};
        pool::thread_pool _pool;
        pool::conditional_wait_for_bool _started{}, _ended{};                               // runner thread terminated.
        std::chrono::nanoseconds _larger_gap = std::chrono::milliseconds(3000); // = 3s
        std::chrono::nanoseconds _wastage = std::chrono::milliseconds(0);
    }; // class timer

    /**
     * @brief a posix timer wrapper class
     * 
     * @par inspired by [https://github.com/Bosma/Scheduler](https://github.com/Bosma/Scheduler) and <https://github.com/jmettraux/rufus-scheduler>.
     */
    template<typename Clock = Clock>
    class ticker : public timer<Clock> {
    public:
        ticker() {}
        ~ticker() {}
        CLAZZ_NON_COPYABLE(ticker);
        using super = timer<Clock>;

        template<typename _Callable, typename... _Args>
        ticker &every(const typename Clock::duration time, _Callable &&f, _Args &&...args) {
            std::shared_ptr<typename super::Job> t = std::make_shared<detail::every_job>(time, std::bind(std::forward<_Callable>(f), std::forward<_Args>(args)...));
            auto next_time = t->next_time_point();
            super::add_task(next_time, std::move(t));
            return (*this);
        }
        template<typename _Callable, typename... _Args>
        ticker &interval(const typename Clock::duration time, _Callable &&f, _Args &&...args) {
            std::shared_ptr<typename super::Job> t = std::make_shared<detail::every_job>(time, std::bind(std::forward<_Callable>(f), std::forward<_Args>(args)...), true);
            super::add_task(Clock::now(), std::move(t));
            return (*this);
        }

        // template<typename _Callable, typename... _Args>
        // ticker &cron(const typename Clock::time_point time, _Callable &&f, _Args &&...args) {
        //     UNUSED(time, f);
        //     return (*this);
        // }
    }; // class ticker

} // namespace hicc::chrono

#endif //HICC_CXX_TICKER_HH
