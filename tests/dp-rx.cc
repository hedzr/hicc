//
// Created by Hedzr Yeh on 2021/9/13.
//

#include "hicc/hz-defs.hh"

#include "hicc/hz-dbg.hh"
#include "hicc/hz-log.hh"
#include "hicc/hz-pipeable.hh"
#include "hicc/hz-string.hh"
#include "hicc/hz-util.hh"
#include "hicc/hz-x-test.hh"

#include <math.h>

#include <iostream>
#include <string>

#include <unordered_map>
#include <vector>

#include <functional>
#include <memory>
#include <random>

namespace hicc::dp::rx::basic {
}

void test_rx_basic() {
    using namespace hicc::dp::rx::basic;
}

void test_traits_head_n() {
    using namespace hicc::dp::rx::basic;
    
    hicc::traits::drop_from_end<3, const char*, double, int, int>::type b{"pi"};
    std::cout << b << '\n';

    hicc::traits::drop_from_end<2, const char*, double, int, int>::type c{"pi", 3.1415};
    detail::print_tuple(std::cout, c) << '\n';

    hicc::traits::head_n<2, const char*, double, int, int>::type hn{"pi", 3.1415};
    detail::print_tuple(std::cout, hn) << '\n';
    hicc::traits::head_n<3, const char*, double, int, int>::type hn3{"pi", 3.1415, 1};
    detail::print_tuple(std::cout, hn3) << '\n';
    
    // **NOTE**
    // compiling passed means head_n/drop_from_end is ok.
    
    // static_assert();
}

int main() {

    HICC_TEST_FOR(test_rx_basic);
    HICC_TEST_FOR(test_traits_head_n);

    return 0;
}