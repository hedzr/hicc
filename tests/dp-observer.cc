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

namespace helpers {

    template<typename... Args>
    inline void Callback(std::function<void(Args...)> f) {
        // store f and call later
        UNUSED(f);
    }

} // namespace helpers

struct moo {
    int doit(int x, int y) { return x + y; }
    bool isBetween(int i, int min, int max) { return i >= min && i <= max; }
};

int doit(int x, int y) { return x + y; }
bool isBetween(int i, int min, int max) { return i >= min && i <= max; }
void printNumber(std::vector<int> &number, std::function<bool(int)> filter) {
    for (const int &i : number) {
        if (filter(i)) {
            std::cout << i << ',';
        }
    }
}

void test_util_bind() {
    using namespace std::placeholders;
    {
        // std::bind

        std::vector<int> numbers{1, 2, 3, 4, 5, 10, 15, 20, 25, 35, 45, 50};
        std::function<bool(int)> filter = std::bind(isBetween, _1, 20, 40);
        printNumber(numbers, filter);
        filter = std::bind([](int i, int min, int max) { return i >= min && i <= max; }, _1, 20, 40);
        printNumber(numbers, filter);
        // filter = bind([](int i, int min, int max) { return i >= min && i <= max; }, _1, 20, 40);
        // printNumber(numbers, filter);

        // lambda_to_function

        helpers::Callback(hicc::types::l2f([](int a, float b) { std::cout << a << ',' << b << '\n'; }));

        // hicc::types::bind

        auto fn0 = hicc::types::bind([](int a, float b) { std::cout << "\nfn0: " << a << ',' << b << '\n'; }, _1, _2);
        fn0(1, 20.f);

        moo m;
        auto fn1 = hicc::types::bind(&moo::doit, m, _1, 3.0f);
        std::cout << "fn1: " << fn1(1) << '\n';

        auto fn2 = hicc::types::bind(doit, _1, 3.0f);
        std::cout << "fn2: " << fn2(9) << '\n';
    }

    {
        auto fn = hicc::types::lambda([&](std::vector<std::string> const &vec) {
            std::cout << vec << '\n';
        });
        std::vector<std::string> vec{"aa", "bb", "cc"};
        std::function<void(std::vector<std::string> const &)> x = fn;
        x(vec);
        hicc_print("lambda fn: %s", std::string(hicc::debug::type_name<decltype(x)>()).c_str());
    }
}

namespace hicc::dp::observer::cb {

    struct event {
        std::string to_string() const { return "event"; }
        // bool operator==(const event &) const { return true; }
        // bool operator==(const std::string_view &tn) const { return hicc::debug::type_name<event>() == tn; }
    };

    struct mouse_move_event : public event {};

    class Store : public hicc::util::observable_bindable<event> {};

} // namespace hicc::dp::observer::cb

void fntest(hicc::dp::observer::cb::event const &e) {
    hicc_print("event CB regular function: %s", e.to_string().c_str());
}

void test_observer_cb() {
    using namespace hicc::dp::observer::cb;
    using namespace std::placeholders;

    Store store;

    auto fn = [](event const &e) {
        hicc_print("event: %s", e.to_string().c_str());
    };
    hicc_print("lambda: %s", std::string(hicc::debug::type_name<decltype(fn)>()).c_str());

    store.add_callback([](event const &e) {
        hicc_print("event CB lamdba: %s", e.to_string().c_str());
    },
                       _1);
    struct eh1 {
        void cb(event const &e) {
            hicc_print("event CB member fn: %s", e.to_string().c_str());
        }
        void operator()(event const &e) {
            hicc_print("event CB member operator() fn: %s", e.to_string().c_str());
        }
    };
    store.on(&eh1::cb, eh1{}, _1);
    store.on(&eh1::operator(), eh1{}, _1);

    store.on(fntest, _1);
    
    store.emit(mouse_move_event{});
}

int main() {

    HICC_TEST_FOR(test_util_bind);
    
    HICC_TEST_FOR(test_observer_basic);
    HICC_TEST_FOR(test_observer_cb);

    return 0;
}