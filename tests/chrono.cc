//
// Created by Hedzr Yeh on 2021/8/5.
//

#include "hicc/hz-chrono.hh"
#include "hicc/hz-process.hh"
#include "hicc/hz-x-test.hh"

#include "hicc/hz-log.hh"

constexpr auto year = 31556952ll; // 格里高利历年的平均秒数

void test_from_cppref() {
    // https://zh.cppreference.com/w/cpp/chrono/duration

    using shakes = std::chrono::duration<int, std::ratio<1, 100'000'000>>;
    using jiffies = std::chrono::duration<int, std::centi>;
    using microfortnights = std::chrono::duration<float, std::ratio<14 * 24 * 60 * 60, 1'000'000>>;
    using nanocenturies = std::chrono::duration<float, std::ratio<100 * year, 1000'000'000>>;

    std::chrono::seconds sec(1);

    std::cout << "1 second is:\n";

    // 无精度损失的整数尺度转换：无转型
    std::cout << '\t' << std::chrono::microseconds(sec).count() << " microseconds\n"
              << '\t' << shakes(sec).count() << " shakes\n"
              << '\t' << jiffies(sec).count() << " jiffies\n";

    // 有精度损失的整数尺度转换：需要转型
    std::cout << '\t' << std::chrono::duration_cast<std::chrono::minutes>(sec).count()
              << " minutes\n";

    // 浮点尺度转换：无转型
    std::cout << '\t' << microfortnights(sec).count() << " microfortnights\n"
              << '\t' << nanocenturies(sec).count() << " nanocenturies\n";
}

void test_traits_is_duration() {
    static_assert(hicc::chrono::is_duration<std::chrono::milliseconds>::value);
    static_assert(hicc::chrono::is_duration_v<std::chrono::milliseconds>);
    
    static_assert(hicc::chrono::is_duration_v<std::thread::id> == false);
}

void foo1() {
    dbg_print("foo1 hit.");
}

void test_c_style(struct timeval &tv) {
#ifdef _WIN32
    // char fmt[64];
    // char buf[256];
    // time_t rawtime;
    // struct tm * timeinfo;
    // time (&rawtime);
    // timeinfo = localtime (&rawtime);
    // strftime(fmt, sizeof(fmt), "%H:%M:%S.%%06u", tm);
    // strftime (buf, sizeof(buf), "Timestamp = %A, %d/%m/%Y %T\0", timeinfo);
    // printf("%s\n", buf);
    // snprintf(buf, sizeof(buf), fmt, tv.tv_usec);
    // printf("%s\n", buf);
    UNUSED(tv);
#else
    char fmt[64];
    char buf[64];
    struct tm *tm;
    tm = localtime(&tv.tv_sec);
    strftime(fmt, sizeof(fmt), "%H:%M:%S.%%06u", tm);
    snprintf(buf, sizeof(buf), fmt, tv.tv_usec);
    printf("%s\n", buf);
#endif
}

void test_cpp_style(std::chrono::system_clock::time_point &now, std::tm &now_tm) {
    //hicc::process::exec date_cmd("date  +\"%T,%N\"");
    {
        using namespace std::chrono;
        // auto now = system_clock::now();
        auto now_ms = time_point_cast<milliseconds>(now);

        auto value = now_ms.time_since_epoch();
        long duration = (long) value.count();

        milliseconds dur(duration);

        time_point<system_clock> dt(dur);

        if (dt != now_ms)
            std::cout << "Failure." << '\n';
        else
            std::cout << "Success." << '\n';
    }

    {
        using namespace std::chrono;
        // Get current time with precision of milliseconds
        auto now11 = time_point_cast<milliseconds>(now); // system_clock::now());
        // sys_milliseconds is type time_point<system_clock, milliseconds>
        using sys_milliseconds = decltype(now11);
        // Convert time_point to signed integral type
        auto integral_duration = now11.time_since_epoch().count();
        // Convert signed integral type to time_point
        sys_milliseconds dt{milliseconds{integral_duration}};
        // test
        if (dt != now11)
            std::cout << "Failure." << '\n';
        else
            std::cout << "Success." << '\n';
    }

    {
        // std::chrono::time_point< std::chrono::system_clock > now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();

        /* UTC: -3:00 = 24 - 3 = 21 */
        typedef std::chrono::duration<int, std::ratio_multiply<std::chrono::hours::period, std::ratio<21>>::type> Days;

        Days days = std::chrono::duration_cast<Days>(duration);
        duration -= days;

        auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
        duration -= hours;

        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
        duration -= minutes;

        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
        duration -= seconds;

        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
        duration -= milliseconds;

        auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration);
        duration -= microseconds;

        auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);

        // C library function - localtime()
        // https://www.tutorialspoint.com/c_standard_library/c_function_localtime.htm
        //
        // struct tm {
        //    int tm_sec;         // seconds,  range 0 to 59
        //    int tm_min;         // minutes, range 0 to 59
        //    int tm_hour;        // hours, range 0 to 23
        //    int tm_mday;        // day of the month, range 1 to 31
        //    int tm_mon;         // month, range 0 to 11
        //    int tm_year;        // The number of years since 1900
        //    int tm_wday;        // day of the week, range 0 to 6
        //    int tm_yday;        // day in the year, range 0 to 365
        //    int tm_isdst;       // daylight saving time
        // };

        time_t theTime = time(NULL);
        struct tm *aTime = localtime(&theTime);

        std::cout << days.count() << " days since epoch or "
                  << days.count() / 365.2524 << " years since epoch. The time is now "
                  << aTime->tm_hour << ":"
                  << minutes.count() << ":"
                  << seconds.count() << ","
                  << milliseconds.count() << ":"
                  << microseconds.count() << ":"
                  << nanoseconds.count() << '\n';
    }

    // std::cout << "date command: " << date_cmd << '\n';

    using iom = hicc::chrono::iom;
    std::cout << iom::fmtflags::gmt << iom::fmtflags::ns << "time_point (ns): os << " << now << '\n';
    std::cout << iom::fmtflags::gmt << iom::fmtflags::us << "time_point (us): os << " << now << '\n';
    std::cout << iom::fmtflags::gmt << iom::fmtflags::ms << "time_point (ms): os << " << now << '\n';
    std::cout << "std::tm:    os << " << now_tm << '\n';

    // {
    //     std::chrono::high_resolution_clock::time_point nowh = std::chrono::high_resolution_clock::now();
    //     std::cout << iom::gmt << iom::ns << "time_point: hires " << nowh << '\n';
    // }

    std::chrono::system_clock::time_point now2 = std::chrono::system_clock::now();
    auto d = now2 - now;
    std::cout << "duration:   os << " << d << '\n';
}

void test_cpp_style(std::chrono::system_clock::time_point &now) {
    std::time_t now_tt = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_tt);
    test_cpp_style(now, now_tm);
}

void test_time_now() {
    struct timeval tv = hicc::chrono::get_system_clock_in_us();
    std::chrono::system_clock::time_point now = hicc::chrono::now();
    auto now1 = hicc::chrono::clock::now();

    test_c_style(tv);
    test_cpp_style(now);

    using iom = hicc::chrono::iom;
    std::cout << iom::fmtflags::local << iom::fmtflags::ns << "time_point (clock): " << now1 << '\n';
    std::cout << iom::fmtflags::local << iom::fmtflags::us << "time_point (clock): " << now1 << '\n';
    std::cout << 1 << '\n'
              << '\n';
}

template<class _Rep, class _Period>
void echo(std::chrono::duration<_Rep, _Period> d) {
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(d);
    std::cout << "duration " << d.count() << " (" << ns.count() << ") = " << d << '\n';
}

void test_format_duration() {
    using namespace std::literals::chrono_literals; // c++14 or later
    for (auto d : std::vector<std::chrono::duration<long double, std::ratio<60>>>{
                 3ns,
                 800ms,
                 59.739us,
                 0.75min,
                 501ns,
                 730us,
                 233ms,
                 7s,
                 7.2s,
                 1024h,
                 5min,
                 5.625min,
                 89.843204843s,
         }) {
        echo(d);
    }
}

void test_try_parse_by() {
    using Clock = std::chrono::system_clock;

    for (auto &time_str : {
             "11:01:37",
#if OS_WIN
                     "1973-1-29 3:59:59", // MSVC: time point before 1970-1-1 is invalid
#else
                        "1937-1-29 3:59:59",
#endif
         }) {
        std::tm tm = hicc::chrono::time_point_2_tm(Clock::now());
        typename Clock::time_point tp;
        if (hicc::chrono::try_parse_by(tm, time_str, "%H:%M:%S", "%Y-%m-%d %H:%M:%S", "%Y/%m/%d %H:%M:%S")) {
            tp = hicc::chrono::tm_2_time_point(&tm);
            dbg_print("  - time '%s' parsed: tp = %s",
                       time_str,
                       // _twl.size(), hit, loop,
                       // hicc::chrono::format_duration(d).c_str(),
                       // hicc::chrono::format_time_point(tp).c_str(),
                       hicc::chrono::format_time_point_to_local(tp).c_str());
        }
    }
}

void test_last_day_at_this_month() {
    namespace chr = hicc::chrono;
    using clock = std::chrono::system_clock;
    using time_point = clock::time_point;
    using namespace std::literals::chrono_literals;

    struct testcase {
        const char *desc;
        int offset;
        time_point now, expected;
    };
#define NOW_CASE(now_str, expected_str, desc, ofs) \
    testcase { desc, ofs, chr::parse_datetime(now_str), chr::parse_datetime(expected_str) }

    for (auto const &t : {
                 // Month .. Year
                 NOW_CASE("2021-08-05", "2021-08-29", "day -3", 3),
                 NOW_CASE("2021-08-05", "2021-08-22", "day -10", 10),
                 NOW_CASE("2021-08-05", "2021-08-17", "day -15", 15),
                 NOW_CASE("2021-08-05", "2021-08-07", "day -25", 25),

         }) {
        auto now = t.now;
        std::tm tm = chr::time_point_2_tm(now);
        tm = chr::last_day_at_this_month(tm, t.offset, 1);
        auto pt = chr::tm_2_time_point(tm);
        dbg_print("%40s: %s -> %s", t.desc, chr::format_time_point_to_local(now).c_str(), chr::format_time_point_to_local(pt).c_str());

        auto tmp = t.expected;
        if (!chr::duration_is_zero(tmp)) {
            if (chr::compare_date_part(pt, tmp) != 0) {
                dbg_print("%40s: ERROR: expecting %s but got %s", " ", chr::format_time_point_to_local(tmp).c_str(), chr::format_time_point_to_local(pt).c_str());
                exit(-1);
            }
        }
    }

#undef NOW_CASE
}

void test_last_day_at_this_year() {
    namespace chr = hicc::chrono;
    using clock = std::chrono::system_clock;
    using time_point = clock::time_point;
    using namespace std::literals::chrono_literals;

    struct testcase {
        const char *desc;
        int offset;
        time_point now, expected;
    };
#define NOW_CASE(now_str, expected_str, desc, ofs) \
    testcase { desc, ofs, chr::parse_datetime(now_str), chr::parse_datetime(expected_str) }

    for (auto const &t : {
                 // Month .. Year
                 NOW_CASE("2021-08-05", "2021-12-29", "day -3", 3),
                 NOW_CASE("2021-08-05", "2021-12-22", "day -10", 10),
                 NOW_CASE("2021-08-05", "2021-12-17", "day -15", 15),
                 NOW_CASE("2021-08-05", "2021-12-07", "day -25", 25),
                 NOW_CASE("2021-08-05", "2021-1-1", "day -365", 365),

         }) {
        auto now = t.now;
        std::tm tm = chr::time_point_2_tm(now);
        tm = chr::last_day_at_this_year(tm, t.offset);
        auto pt = chr::tm_2_time_point(tm);
        dbg_print("%40s: %s -> %s", t.desc, chr::format_time_point_to_local(now).c_str(), chr::format_time_point_to_local(pt).c_str());

        auto tmp = t.expected;
        if (!chr::duration_is_zero(tmp)) {
            if (chr::compare_date_part(pt, tmp) != 0) {
                dbg_print("%40s: ERROR: expecting %s but got %s", " ", chr::format_time_point_to_local(tmp).c_str(), chr::format_time_point_to_local(pt).c_str());
                exit(-1);
            }
        }
    }

#undef NOW_CASE
}

int main() {
    HICC_TEST_FOR(test_try_parse_by);
    HICC_TEST_FOR(test_time_now);
    HICC_TEST_FOR(test_format_duration);

    HICC_TEST_FOR(test_last_day_at_this_year);
    HICC_TEST_FOR(test_last_day_at_this_month);

    HICC_TEST_FOR(test_from_cppref);
    HICC_TEST_FOR(test_traits_is_duration);

#ifndef _WIN32
    {
        struct timespec ts;
        clock_getres(CLOCK_REALTIME, &ts);
        std::cout << ts.tv_sec << ',' << ts.tv_nsec << '\n';
        clock_getres(CLOCK_MONOTONIC, &ts);
        std::cout << ts.tv_sec << ',' << ts.tv_nsec << '\n';
    }
#endif
}