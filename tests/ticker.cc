//
// Created by Hedzr Yeh on 2021/7/13.
//

// #define HICC_TEST_THREAD_POOL_DBGOUT 1

// #define HICC_ENABLE_THREAD_POOL_READY_SIGNAL 1

#include "hicc/hz-ticker.hh"
#include "hicc/hz-x-class.hh"
#include "hicc/hz-x-test.hh"

#include <chrono>
#include <iostream>
#include <thread>

hicc::debug::X x_global_var;

namespace test {
    template<typename T>
    constexpr auto ZFN() {
        constexpr auto function = std::string_view{__FUNCTION_NAME__};
        constexpr auto prefix = std::string_view{"auto ZFN() [T = "};
        constexpr auto suffix = std::string_view{"]"};

        constexpr auto start = function.find(prefix) + prefix.size();
        constexpr auto end = function.rfind(suffix);
        std::string_view name = function.substr(start, (end - start));
        return name.substr(0, (end - start));
        // return function;
    }

    template<typename T>
    constexpr auto ZFS() {
        constexpr auto function = std::string_view{__FUNCTION_NAME__};
        constexpr auto prefix = std::string_view{"auto ZFT() [T = "};
        // constexpr auto suffix = std::string_view{"]"};

        constexpr auto start = function.find(prefix) + prefix.size();
        // constexpr auto end = function.rfind(suffix);
        return (unsigned long long) start; // function.substr(start, (end - start));
        // return function;
    }

    template<typename T>
    constexpr auto ZFT() {
        constexpr auto function = std::string_view{__FUNCTION_NAME__};
        // constexpr auto prefix = std::string_view{"auto ZFT() [T = "};
        constexpr auto suffix = std::string_view{"]"};

        // constexpr auto start = function.find(prefix) + prefix.size();
        constexpr auto end = function.rfind(suffix);
        return (unsigned long long) end; // function.substr(start, (end - start));
        // return function;
    }

    template<typename T>
    constexpr auto ZFZ() {
        // auto ZFZ() [T = std::__1::basic_string<char>]
        constexpr auto function = std::string_view{__FUNCTION_NAME__};
        return function;
    }
} // namespace test

void test_type_name() {
    printf(">>Z '%s'\n", test::ZFZ<std::string>().data());
    printf(">>Z '%s'\n", test::ZFN<std::string>().data());
    printf(">>Z %llu, %llu\n", test::ZFS<std::string>(), test::ZFT<std::string>());

#ifndef _WIN32
    printf(">>2 '%s'\n", hicc::debug::type_name_holder<std::string>::value.data());

    printf(">>1 '%s'\n", hicc::debug::type_name_1<hicc::pool::conditional_wait_for_int>().data());
    printf(">>> '%s'\n", hicc::debug::type_name<hicc::pool::conditional_wait_for_int>().data());
#endif

    auto fn = hicc::debug::type_name<std::string>();
    std::string str{fn};
    printf(">>> '%s'\n", str.c_str());

    std::cout << hicc::debug::type_name<std::string>() << '\n';
    std::cout << std::string(hicc::debug::type_name<std::string>()) << '\n';
    printf(">>> %s\n", std::string(hicc::debug::type_name<std::string>()).c_str());
}

void foo() {
    std::cout << x_global_var.c_str() << '\n';
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void test_thread_basics() {
    printf("> %s\n", __FUNCTION_NAME__);
    {
        std::thread t;
        std::cout << "- before starting, joinable: " << std::boolalpha << t.joinable()
                  << '\n';

        t = std::thread(foo);
        std::cout << "- after starting, joinable: " << t.joinable()
                  << '\n';

        t.join();
        std::cout << "- after joining, joinable: " << t.joinable()
                  << '\n';
    }
    {
        unsigned int n = std::thread::hardware_concurrency();
        std::cout << "- " << n << " concurrent _threads are supported.\n";
    }
}

void foo1() {
    hicc_print("foo1 hit.");
}

void test_periodical_job() {
    namespace chr = hicc::chrono;
    namespace dtl = hicc::chrono::detail;
    using clock = std::chrono::system_clock;
    using time_point = clock::time_point;
    using namespace std::literals::chrono_literals;

    struct testcase {
        const char *desc;
        chr::anchors anchor;
        int offset;
        int ordinal;
        int times;
        time_point now, expected;
    };
#define CASE(desc, anchor, ofs) \
    testcase { desc, anchor, ofs, 1, -1, time_point(), time_point() }
#define CASE1(desc, anchor, ofs, ord) \
    testcase { desc, anchor, ofs, ord, -1, time_point(), time_point() }
#define CASE2(desc, anchor, ofs, ord, times) \
    testcase { desc, anchor, ofs, ord, times, time_point(), time_point() }
#define NOW_CASE(now_str, expected_str, desc, anchor, ofs) \
    testcase { desc, anchor, ofs, 1, -1, hicc::chrono::parse_datetime(now_str), hicc::chrono::parse_datetime(expected_str) }

    for (auto t : {
                 // Month
                 NOW_CASE("2021-08-05", "2021-09-03", "day 3 every month", chr::anchors::Month, 3),
                 NOW_CASE("2021-08-05", "2021-08-23", "day 23 every month", chr::anchors::Month, 23),
                 NOW_CASE("2021-08-05", "2021-08-29", "day -3 every month", chr::anchors::Month, -3),
                 NOW_CASE("2021-08-05", "2021-08-17", "day -15 every month", chr::anchors::Month, -15),
                 NOW_CASE("2021-08-17", "2021-09-17", "day -15 every month", chr::anchors::Month, -15),

                 // Year
                 NOW_CASE("2021-08-05", "2022-08-03", "day 3 (this month) every year", chr::anchors::Year, 3),
                 NOW_CASE("2021-08-05", "2021-08-23", "day 23 (this month) every year", chr::anchors::Year, 23),
                 NOW_CASE("2021-08-05", "2021-12-29", "day -3 (this month) every year", chr::anchors::Year, -3),
                 NOW_CASE("2021-08-05", "2021-12-17", "day -15 (this month) every year", chr::anchors::Year, -15),
                 NOW_CASE("2021-08-18", "2021-12-17", "day -15 (this month) every year", chr::anchors::Year, -15),

                 // FirstThirdOfMonth ...
                 NOW_CASE("2021-08-05", "2021-09-03", "day 3 every first third of month", chr::anchors::FirstThirdOfMonth, 3),
                 NOW_CASE("2021-08-05", "2021-08-08", "day 8 every first third of month", chr::anchors::FirstThirdOfMonth, 8),
                 NOW_CASE("2021-08-05", "2021-08-08", "day -3 every first third of month", chr::anchors::FirstThirdOfMonth, -3),
                 NOW_CASE("2021-08-05", "2021-09-04", "day -7 every first third of month", chr::anchors::FirstThirdOfMonth, -7),

                 // MiddleThirdOfMonth ...
                 NOW_CASE("2021-08-15", "2021-09-13", "day 3 every mid third of month", chr::anchors::MiddleThirdOfMonth, 3),
                 NOW_CASE("2021-08-15", "2021-08-18", "day 8 every mid third of month", chr::anchors::MiddleThirdOfMonth, 8),
                 NOW_CASE("2021-08-15", "2021-08-18", "day -3 every mid third of month", chr::anchors::MiddleThirdOfMonth, -3),
                 NOW_CASE("2021-08-15", "2021-09-14", "day -7 every mid third of month", chr::anchors::MiddleThirdOfMonth, -7),

                 // LastThirdOfMonth ...
                 NOW_CASE("2021-08-25", "2021-09-23", "day 3 every last third of month", chr::anchors::LastThirdOfMonth, 3),
                 NOW_CASE("2021-08-25", "2021-08-28", "day 8 every last third of month", chr::anchors::LastThirdOfMonth, 8),
                 NOW_CASE("2021-08-25", "2021-08-29", "day -3 every last third of month", chr::anchors::LastThirdOfMonth, -3),
                 NOW_CASE("2021-08-25", "2021-09-23", "day -9 every last third of month", chr::anchors::LastThirdOfMonth, -9),
                 NOW_CASE("2021-08-22", "2021-08-23", "day -9 every last third of month", chr::anchors::LastThirdOfMonth, -9),

         }) {
        dtl::periodical_job pj(t.anchor, t.ordinal, t.offset, t.times, foo1);
        auto now = t.now;
        if (chr::duration_is_zero(now))
            now = hicc::chrono::now();
        else
            pj.last_pt = now;
        auto pt = pj.next_time_point(now);
        hicc_print("%40s: %s -> %s", t.desc, hicc::chrono::format_time_point_to_local(now).c_str(), hicc::chrono::format_time_point_to_local(pt).c_str());

        auto tmp = t.expected;
        if (!chr::duration_is_zero(tmp)) {
            if (hicc::chrono::compare_date_part(pt, tmp) != 0) {
                hicc_print("%40s: ERROR: expecting %s but got %s", " ", hicc::chrono::format_time_point_to_local(tmp).c_str(), hicc::chrono::format_time_point_to_local(pt).c_str());
                exit(-1);
            }
        }
    }

#undef NOW_CASE
}

void test_timer() {
    using namespace std::literals::chrono_literals;
    hicc::debug::X x_local_var;

    hicc::pool::conditional_wait_for_int count{1};
    hicc::chrono::timer t;
#if !HICC_ENABLE_THREAD_POOL_READY_SIGNAL
    std::this_thread::sleep_for(300ms);
#endif

    {
        auto now = hicc::chrono::now();
        printf("  - start at: %s\n", hicc::chrono::format_time_point(now).c_str());
    }
    t.after(100ms, [&count]() {
        auto now = hicc::chrono::now();
        // std::time_t ct = std::time(0);
        // char *cc = ctime(&ct);
        printf("  - after [%02d]: %s\n", count.val(), hicc::chrono::format_time_point(now).c_str());
        count.set();
    });

    count.wait();
    // t.clear();
}

void test_ticker() {
    using namespace std::literals::chrono_literals;

    hicc::pool::conditional_wait_for_int count{16};
    hicc::chrono::ticker t;
#if !HICC_ENABLE_THREAD_POOL_READY_SIGNAL
    std::this_thread::sleep_for(300ms);
#endif

    {
        auto now = hicc::chrono::now();
        printf("  - start at: %s\n", hicc::chrono::format_time_point(now).c_str());
    }
    t.every(300ms, [&count]() {
        auto now = hicc::chrono::now();
        // std::time_t ct = std::time(0);
        // char *cc = ctime(&ct);
        printf("  - every [%02d]: %s\n", count.val(), hicc::chrono::format_time_point(now).c_str());
        count.set();
    });

    count.wait();
    // while (count < 3) {
    //     std::this_thread::yield();
    // }
    // std::this_thread::sleep_for(1s);
    t.clear();
}

int main() {
    // test_thread();

    HICC_TEST_FOR(test_periodical_job);

#if 0
    HICC_TEST_FOR(test_type_name);

    HICC_TEST_FOR(test_thread_basics);

    HICC_TEST_FOR(test_timer);
    HICC_TEST_FOR(test_ticker);

    // using namespace std::literals::chrono_literals;
    // std::this_thread::sleep_for(200ns);
    //
    // {
    //     std::time_t ct = std::time(0);
    //     char *cc = ctime(&ct);
    //     printf(". now: %s\n", cc);
    // }
    // {
    //     auto t = std::time(nullptr);
    //     auto tm = *std::localtime(&t);
    //     std::cout << std::put_time(&tm, "%d-%m-%Y %H-%M-%S") << '\n';
    // }
#endif
    return 0;
}