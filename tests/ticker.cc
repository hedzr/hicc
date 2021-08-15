//
// Created by Hedzr Yeh on 2021/7/13.
//

#define HICC_TEST_THREAD_POOL_DBGOUT 1
// #define HICC_ENABLE_THREAD_POOL_READY_SIGNAL 1

#include "hicc/hz-ticker.hh"
#include "hicc/hz-x-class.hh"
#include "hicc/hz-x-test.hh"

#include <chrono>
#include <iostream>
#include <thread>

hicc::debug::X x_global_var;

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

int main() {
    // test_thread();

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
    return 0;
}