//
// Created by Hedzr Yeh on 2021/8/29.
//

#include "hicc/hz-defs.hh"

#include "hicc/hz-dbg.hh"
#include "hicc/hz-log.hh"
#include "hicc/hz-pipeable.hh"
#include "hicc/hz-pool.hh"
#include "hicc/hz-x-class.hh"
#include "hicc/hz-x-test.hh"

#include <iostream>
#include <math.h>
#include <string>

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

int main() {

    HICC_TEST_FOR(test_type_name);

    return 0;
}