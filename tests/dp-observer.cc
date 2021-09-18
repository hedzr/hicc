//
// Created by Hedzr Yeh on 2021/9/14.
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

namespace hicc::dp::observer::basic {

    struct event {
        // bool operator==(const event &) const { return true; }
        // bool operator==(const std::string_view &tn) const { return hicc::debug::type_name<event>() == tn; }
    };

    struct mouse_move_event : public event {};
    
    class Store : public hicc::util::observable<event, true> {};

    class Customer : public hicc::util::observer<event> {
    public:
        virtual ~Customer() {}
        bool operator==(const Customer &r) const { return this == &r; }
        void observe(const subject_t &) override {
            hicc_debug("event raised: %s", debug::type_name<subject_t>().data());
        }
    };

} // namespace hicc::dp::observer::basic

void test_observer_basic() {
    using namespace hicc::dp::observer::basic;

    Store store;
    Store::observer_t_shared c = std::make_shared<Customer>(); // uses Store::observer_t_shared rather than 'auto'

    store += c;
    store.emit(event{});
    store -= c;

    hicc::util::registerer<event, true> __r(store, c);
    store.emit(mouse_move_event{});
}

int main() {

    HICC_TEST_FOR(test_observer_basic);

    return 0;
}