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
        Clock::time_point next_time_point() const { return next_time_point(Clock::now()); }
        virtual Clock::time_point next_time_point(Clock::time_point const now) const = 0;

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
            virtual ~in_job() {}

            // dummy time_point because it's not used
            Clock::time_point next_time_point(Clock::time_point const) const override { return Clock::time_point(Clock::duration(0)); }
        };

        class every_job : public timer_job {
        public:
            explicit every_job(Clock::duration d, std::function<void()> &&f, bool interval = false)
                : timer_job(std::move(f), true, interval)
                , dur(d) {}
            virtual ~every_job() {}

            Clock::time_point next_time_point(Clock::time_point const now) const override {
#if defined(_DEBUG) || HICC_TEST_THREAD_POOL_DBGOUT
                auto nxt = now + dur;
                pool_debug("         %s -> %s", chrono::format_time_point(now).c_str(), chrono::format_time_point(nxt).c_str());
                return nxt;
#else
                return now + dur;
#endif
            };

            Clock::duration dur;
        };

    } // namespace detail

    enum class anchors {
        Nothing,

        Month,       // 'offset' (1..31, -1..-31) day every month; with 'ordinal' times
        TwoMonth,    // 'offset' day every two month
        Quarter,     // 'offset' quarter day every quarter
        FourMonth,   //
        FiveMonth,   //
        SixMonth,    // 'offset' day every half a year
        SevenMonth,  //
        EightMonth,  // eight
        NineMonth,   //
        TenMonth,    // ten
        ElevenMonth, //
        Year,        // 'offset' day every year, this month; with 'ordinal' times

        FirstThirdOfMonth,  // 'offset' (0..9, -1..-10) day 在每一个上旬; // with 'ordinal' times
        MiddleThirdOfMonth, // 'offset' (0..9, -1..-10) day 在每一个中旬; // with 'ordinal' times
        LastThirdOfMonth,   // 'offset' (0..9, -1..-10) day 在每一个下旬; // with 'ordinal' times

        DayInYear,   // 'offset' (0-365) day every one year
        WeekInMonth, // 'offset' (0..5)  week every one month
        WeekInYear,  // 'offset' (0..51) week every one year
        Week,        // 'offset' (0..6)  day every one week

        Dummy00000 = 1000,
        // Hour,
        // Minute,
        // Second,
        // Millisecond,
        // Microsecond,
        // Nanosecond,

        Dummy00001 = 2000,
        Dummy00009 = 9000,
    };

    namespace detail {

        template<typename Clock = std::chrono::system_clock, bool GMT = false>
        class periodical_job : public timer_job {
        public:
            explicit periodical_job(anchors anchor_, int ordinal_, int offset_, int times_, std::function<void()> &&f, bool interval = false)
                : timer_job(std::move(f), true, interval)
                , last_pt(Clock::now())
                , anchor(anchor_)
                , ordinal(ordinal_)
                , offset(offset_)
                , times(times_) {}
            virtual ~periodical_job() {}

            typename Clock::time_point next_time_point(typename Clock::time_point const now) const override {
                if (now < last_pt)
                    return last_pt;

                typename Clock::time_point pt;
                auto time_now = Clock::to_time_t(now);
                std::tm tm = GMT ? *std::gmtime(&time_now) : *std::localtime(&time_now);

                switch (anchor) {
                    case anchors::Nothing:
                        break;

                    case anchors::Month:
                    case anchors::TwoMonth:
                    case anchors::Quarter:
                    case anchors::FourMonth:
                    case anchors::FiveMonth:
                    case anchors::SixMonth:
                    case anchors::SevenMonth:
                    case anchors::EightMonth:
                    case anchors::NineMonth:
                    case anchors::TenMonth:
                    case anchors::ElevenMonth:
                    case anchors::Year: {
                        int delta = ordinal * (int) anchor;
                        if (offset > 0) {
                            if (tm.tm_mday >= offset) {
                                tm.tm_mon += delta;
                            }
                            tm.tm_mday = offset;
                            while (tm.tm_mon > 11) {
                                tm.tm_mon -= 12;
                                tm.tm_year++;
                            }
                        } else if (anchor < anchors::Year) {
                            int ofs = -offset;
                            tm = chrono::last_day_at_this_month<Clock, GMT>(tm, ofs, delta);
                        } else {
                            int ofs = -offset;
                            tm = chrono::last_day_at_this_year<Clock, GMT>(tm, ofs);
                        }
                        pt = chrono::tm_2_time_point<Clock>(&tm);
                        if (pt <= now) {
                            tm.tm_mon++;
                            pt = chrono::tm_2_time_point<Clock>(&tm);
                        }
                    } break;

                    case anchors::FirstThirdOfMonth: {
                        int delta = 1 * ordinal;
                        int day = offset > 0 ? offset : 11 + offset;
                        if (tm.tm_mday >= day) {
                            tm.tm_mon += delta;
                        }
                        tm.tm_mday = day;
                        pt = chrono::tm_2_time_point<Clock>(&tm);

                    } break;
                    case anchors::MiddleThirdOfMonth: {
                        int delta = 1 * ordinal;
                        int day = offset > 0 ? 10 + offset : 21 + offset;
                        if (tm.tm_mday >= day) {
                            tm.tm_mon += delta;
                        }
                        tm.tm_mday = day;
                        pt = chrono::tm_2_time_point<Clock>(&tm);
                    } break;
                    case anchors::LastThirdOfMonth: {
                        int delta = 1 * ordinal;
                        if (offset > 0) {
                            int day = 20 + offset;
                            if (tm.tm_mday >= day) {
                                tm.tm_mon += delta;
                            }
                            tm.tm_mday = day;
                        } else {
                            int ofs = -offset;
                            std::tm tmp = chrono::last_day_at_this_month<Clock, GMT>(tm, ofs, delta - 1);
                            if (tmp.tm_mday >= ofs) {
                                tm = chrono::last_day_at_this_month<Clock, GMT>(tm, ofs, delta);
                            } else {
                                tm = tmp;
                                tm.tm_mday -= (ofs);
                            }
                        }
                        pt = chrono::tm_2_time_point<Clock>(&tm);
                        if (pt < last_pt) {
                            tm.tm_mon++;
                            pt = chrono::tm_2_time_point<Clock>(&tm);
                        }
                    } break;

                    case anchors::DayInYear: { // 'offset': which day (0..365) in a year
                        typename Clock::time_point t;
                        int ofs;
                        if (offset > 0) {
                            ofs = offset;
                            t = chrono::tm_2_time_point<Clock>(&tm);
                        } else {
                            ofs = -offset;
                            std::tm tmp = chrono::last_day_at_this_year<Clock, GMT>(tm, ofs);
                            t = chrono::tm_2_time_point<Clock>(&tmp);
                            tm = tmp;
                        }

                        if (tm.tm_yday > ofs) {
                            int day_delta = ofs - tm.tm_wday;
                            t += std::chrono::hours(day_delta * 24);
                        } else {
                            int day_delta = ordinal + tm.tm_yday + 1 - tm.tm_wday;
                            t += std::chrono::hours(day_delta * 24);
                        }
                        tm = chrono::time_point_2_tm<Clock, GMT>(t);

                        pt = chrono::tm_2_time_point<Clock>(&tm);
                    } break;

                    case anchors::WeekInMonth: {
                        // 'offset': which week in a month
                        // 'ordinal': weekday in that week
                        if (offset > 0) {
                            int ofs = offset == 0 ? 1 : offset;
                            // std::tm tmp = chrono::last_day_at_this_month(tm, ofs, delta);
                            std::tm tmp = tm;
                            tmp.tm_mday = 1; // get 1st day in this month
                            auto t = chrono::tm_2_time_point<Clock>(&tmp);
                            tmp = chrono::time_point_2_tm<Clock, GMT>(t);

                            if (tmp.tm_wday < ordinal) {
                                int day_delta = ordinal - tmp.tm_wday;
                                t += std::chrono::hours(day_delta * 24);
                            } else {
                                int day_delta = ordinal + 7 - tmp.tm_wday;
                                t += std::chrono::hours(day_delta * 24);
                                ofs--;
                            }
                            ofs--;
                            if (ofs > 0) {
                                t += std::chrono::hours(ofs * 7 * 24);
                            }

                            tm = chrono::time_point_2_tm<Clock, GMT>(t);
                        } else {
                            int ofs = -offset;
                            std::tm tmp = chrono::last_day_at_this_month<Clock, GMT>(tm, ofs, 1);

                            typename Clock::time_point t;
                            if (tmp.tm_wday < ordinal) {
                                int day_delta = ordinal - tmp.tm_wday;
                                t -= std::chrono::hours(day_delta * 24);
                            } else {
                                int day_delta = ordinal + 7 - tmp.tm_wday;
                                t -= std::chrono::hours(day_delta * 24);
                                ofs--;
                            }
                            ofs--;
                            if (ofs > 0) {
                                t -= std::chrono::hours(ofs * 7 * 24);
                            }

                            tm = chrono::time_point_2_tm<Clock, GMT>(t);
                        }
                        pt = chrono::tm_2_time_point<Clock>(&tm);
                    } break;

                    case anchors::WeekInYear: {
                        // 'offset': which week in a year
                        // 'ordinal': weekday in that week
                        if (offset > 0) {
                            int ofs = offset == 0 ? 1 : offset;
                            std::tm tmp = tm;
                            tmp.tm_mday = 1; // get 1st day in this year
                            tmp.tm_mon = 0;
                            auto t = chrono::tm_2_time_point<Clock>(&tmp);
                            tmp = chrono::time_point_2_tm<Clock, GMT>(t);

                            if (tmp.tm_wday < ordinal) {
                                int day_delta = ordinal - tmp.tm_wday;
                                t += std::chrono::hours(day_delta * 24);
                            } else {
                                int day_delta = ordinal + 7 - tmp.tm_wday;
                                t += std::chrono::hours(day_delta * 24);
                                ofs--;
                            }
                            ofs--;
                            if (ofs > 0) {
                                t += std::chrono::hours(ofs * 7 * 24);
                            }

                            tm = chrono::time_point_2_tm<Clock, GMT>(t);
                        } else {
                            int ofs = -offset;
                            std::tm tmp = chrono::last_day_at_this_year<Clock, GMT>(tm, ofs);

                            typename Clock::time_point t;
                            if (tmp.tm_wday < ordinal) {
                                int day_delta = ordinal - tmp.tm_wday;
                                t -= std::chrono::hours(day_delta * 24);
                            } else {
                                int day_delta = ordinal + 7 - tmp.tm_wday;
                                t -= std::chrono::hours(day_delta * 24);
                                ofs--;
                            }
                            ofs--;
                            if (ofs > 0) {
                                t -= std::chrono::hours(ofs * 7 * 24);
                            }

                            tm = chrono::time_point_2_tm<Clock, GMT>(t);
                        }
                        pt = chrono::tm_2_time_point<Clock>(&tm);
                    } break;

                    case anchors::Week: {
                        int day_delta;
                        if (offset > 0) {
                            day_delta = tm.tm_wday > offset ? tm.tm_wday - offset : offset - tm.tm_wday + 7;
                        } else {
                            int ofs = 7 + offset;
                            day_delta = tm.tm_wday > ofs ? tm.tm_wday - ofs : ofs - tm.tm_wday + 7;
                        }
                        pt = now + std::chrono::hours(day_delta * 24);
                    } break;

                    case anchors::Dummy00000:
                        break;
                        // case anchors::Hour:
                        //     pt = chrono::tm_2_time_point<Clock ,GMT>(&tm);
                        //     break;
                        // case anchors::Minute:
                        //     pt = chrono::tm_2_time_point<Clock ,GMT>(&tm);
                        //     break;
                        // case anchors::Second:
                        //     pt = chrono::tm_2_time_point<Clock ,GMT>(&tm);
                        //     break;
                        // case anchors::Millisecond:
                        //     pt = chrono::tm_2_time_point<Clock ,GMT>(&tm);
                        //     break;
                        // case anchors::Microsecond:
                        //     pt = chrono::tm_2_time_point<Clock ,GMT>(&tm);
                        //     break;
                        // case anchors::Nanosecond:
                        //     pt = chrono::tm_2_time_point<Clock ,GMT>(&tm);
                        //     break;

                    case anchors::Dummy00001:
                        break;
                    case anchors::Dummy00009:
                        break;
                }

                (const_cast<periodical_job *>(this)->last_pt) = pt;
                // pool_debug("         %s -> %s", chrono::format_time_point(now).c_str(), chrono::format_time_point(nxt).c_str());
                return last_pt;
            };

            typename Clock::time_point last_pt;
            anchors anchor = anchors::Nothing; // 0: no anchor, 1: month, 2: quarter, 3: half a year, 4: year,
            int ordinal = 1;                   // ordinal in anchor
            int offset = 1;                    // >0: from start, <0: before end
            int times = -1;                    // repeat time. -1: no limit
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
            std::tm tm;
            // auto time_now = Clock::to_time_t(Clock::now());
            // std::tm tm = *std::localtime(&time_now);
            // our final time as a time_point
            typename Clock::time_point tp;
            if (hicc::chrono::try_parse_by(tm, time, "%H:%M:%S", "%Y-%m-%d %H:%M:%S", "%Y/%m/%d %H:%M:%S")) {
                tp = hicc::chrono::tm_2_time_point(&tm);
                // if we've already passed this time, the user will mean next day, so add a day.
                if (Clock::now() >= tp)
                    tp += std::chrono::hours(24);
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
        pool::conditional_wait_for_bool _started{}, _ended{};                   // runner thread terminated.
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
