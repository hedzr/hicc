//
// Created by Hedzr Yeh on 2021/8/5.
//

#include "hicc/hz-chrono.hh"
#include "hicc/hz-process.hh"

void test_c_style(struct timeval &tv) {
    char fmt[64];
    char buf[64];
    struct tm *tm;
    tm = localtime(&tv.tv_sec);
    strftime(fmt, sizeof(fmt), "%H:%M:%S.%%06u", tm);
    snprintf(buf, sizeof(buf), fmt, tv.tv_usec);
    printf("%s\n", buf);
}

void test_cpp_style(std::chrono::system_clock::time_point &now, std::tm &now_tm) {
    //hicc::process::exec date_cmd("date  +\"%T,%N\"");
    {
        using namespace std::chrono;
        // auto now = system_clock::now();
        auto now_ms = time_point_cast<milliseconds>(now);

        auto value = now_ms.time_since_epoch();
        long duration = value.count();

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
    std::cout << iom::gmt << iom::ns << "time_point (ns): os << " << now << '\n';
    std::cout << iom::gmt << iom::us << "time_point (us): os << " << now << '\n';
    std::cout << iom::gmt << iom::ms << "time_point (ms): os << " << now << '\n';
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
    std::cout << iom::local << iom::ns << "time_point (clock): " << now1 << '\n';
    std::cout << iom::local << iom::us << "time_point (clock): " << now1 << '\n';
    std::cout << 1 << '\n'
              << '\n';
}

template<class _Rep, class _Period>
void echo(std::chrono::duration<_Rep, _Period> d) {
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(d);
    std::cout << "duration " << d.count() << " (" << ns.count() << ") = " << d << '\n';
}

void test_format_duration() {
    using namespace std::literals::chrono_literals;
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
                 89.843204843s,
         }) {
        echo(d);
    }
}

int main() {
    test_time_now();
    test_format_duration();

    struct timespec ts;
    clock_getres(CLOCK_REALTIME, &ts);
    std::cout << ts.tv_sec << ',' << ts.tv_nsec << '\n';
    clock_getres(CLOCK_MONOTONIC, &ts);
    std::cout << ts.tv_sec << ',' << ts.tv_nsec << '\n';
}