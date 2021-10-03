//
// Created by Hedzr Yeh on 2021/9/13.
//

#include <algorithm>
#include <functional>
#include <memory>
#include <new>
#include <random>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <utility>

#include <atomic>
#include <condition_variable>
#include <mutex>

#include <any>
#include <array>
#include <chrono>
#include <deque>
#include <initializer_list>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#include <math.h>

#include "hicc/hz-defs.hh"

#include "hicc/hz-dbg.hh"
#include "hicc/hz-log.hh"
#include "hicc/hz-pipeable.hh"
#include "hicc/hz-string.hh"
#include "hicc/hz-util.hh"
#include "hicc/hz-x-test.hh"

namespace hicc::dp::strategy::basic {

    struct guide {};
    struct context {};

    class router {
    public:
        virtual ~router() {}
        virtual guide make_guide(context &ctx) = 0;
    };

    template<typename T>
    class router_t : public router {
    public:
        virtual ~router_t() {}
    };

    class by_walk : public router_t<by_walk> {
    public:
        virtual ~by_walk() {}
        guide make_guide(context &ctx) override {
            guide g;
            UNUSED(ctx);
            return g;
        }
    };

    class by_transit : public router_t<by_transit> {
    public:
        virtual ~by_transit() {}
        guide make_guide(context &ctx) override {
            guide g;
            UNUSED(ctx);
            return g;
        }
    };

    class by_drive : public router_t<by_drive> {
    public:
        virtual ~by_drive() {}
        guide make_guide(context &ctx) override {
            guide g;
            UNUSED(ctx);
            return g;
        }
    };

    class route_guide {
    public:
        void guide_it(router &strategy) {
            context ctx;
            guide g = strategy.make_guide(ctx);
            print(g);
        }

    protected:
        void print(guide &g) {
            UNUSED(g);
        }
    };

} // namespace hicc::dp::strategy::basic

void test_strategy_basic() {
    using namespace hicc::dp::strategy::basic;
    route_guide rg;

    by_walk s;
    rg.guide_it(s);
}

void test_traits_head_n() {
    using namespace hicc::dp::strategy::basic;

    hicc::traits::drop_from_end<3, const char *, double, int, int>::type b{"pi"};
    std::cout << b << '\n';

    hicc::traits::drop_from_end<2, const char *, double, int, int>::type c{"pi", 3.1415};
    detail::print_tuple(std::cout, c) << '\n';

    hicc::traits::head_n<2, const char *, double, int, int>::type hn{"pi", 3.1415};
    detail::print_tuple(std::cout, hn) << '\n';
    hicc::traits::head_n<3, const char *, double, int, int>::type hn3{"pi", 3.1415, 1};
    detail::print_tuple(std::cout, hn3) << '\n';

    // **NOTE**
    // compiling passed means head_n/drop_from_end is ok.

    // static_assert();
}

int main() {

    HICC_TEST_FOR(test_strategy_basic);
    HICC_TEST_FOR(test_traits_head_n);

    return 0;
}