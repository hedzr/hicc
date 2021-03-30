//
// Created by Hedzr Yeh on 2021/3/1.
//

#include "hicc/hz-btree.hh"
#include "hicc/hz-chrono.hh"

#include <cstdio>
#include <iomanip>
#include <iostream>

#include <cstdlib>
#include <ctime>

#include <cmath>
#include <fstream>
#include <memory>
#include <random>


void n_test_btree() {

    auto source = {9, 11, 2, 7, 3, 5, 13, 17, 19, -19, 19, 21, 6, 8, 12, 29, 31, 18, 20, 16, 14, 15, -15, 15, 1, -1, 1, 10, 33, 47, 28, 55, 56, 66, 78, 88, 89, 69, -69, 69};

    hicc::chrono::high_res_duration hrd([](auto duration) -> bool {
        std::cout << "test_btree_even<4>: It took " << duration << '\n';
        return false;
    });

    using btree = hicc::btree::btree<int, std::less<int>>;
    btree bt;
    // bt.dot_prefix("tree");
    for (auto const &v : source) {
        if (v > 0) {
            if (v == 5)
                std::cout << v << '\n';
            bt.insert(v);
            if (v == 19 && false) {
                bt.remove(19);
                bt.insert(19);
                exit(0);
            }
        } else {
            if (v == -69)
                std::cout << v << '\n';
            bt.remove(-v);
        }
        // NO_ASSERTIONS_ONLY(bt.dbg_dump());
    }
    assert(bt.exists(11));
    auto [ok, node, idx] = bt.find(11);
    std::cout << "pos of " << 11 << ": [" << node.to_string() << "] #" << idx << '\n';

    bt.insert(300, 350, 370);
    bt.remove(88, 370);

    std::cout << "btree: " << vector_to_string(bt.to_vector()) << '\n';
    std::cout << "btree: " << bt << '\n';

    std::list<int> vec2 = {5, 9, 19};
    std::cout << vector_to_string(vec2) << '\n';
}


void n_test_btree_rand(int count = 100 * 1000) {
    hicc::chrono::high_res_duration hrd;

    std::cout << '\n'
              << "random test..." << '\n'
              << '\n';

    if (count <= 0) count = 200;

    const int MAX = 32768;
    std::random_device r;
    std::default_random_engine e1(r());
    std::uniform_int_distribution<int> uniform_dist(1, MAX);

    std::srand((unsigned int) std::time(nullptr)); // use current time as seed for random generator

    using btree = hicc::btree::btree<int, std::less<int>>;
    std::vector<int> series;
    btree bt;
    // bt.dot_prefix("rand");
    for (int ix = 0; ix < (count); ix++) {
        // int choice = std::rand();
        int choice = uniform_dist(e1);
        if (choice > MAX / 3) {
            // int random_variable = std::rand();
            int random_variable = uniform_dist(e1);

            if (!bt.exists(random_variable)) {
                series.push_back(random_variable);
                std::cout << "series(" << series.size() << "): " << series << '\n';
                bt.insert(random_variable);
            }
        } else {
            // int index1 = std::rand();
            // auto index = (int) (((double) index1) / RAND_MAX * bt.size());
            std::uniform_int_distribution<int> uniform_dist_s(0, (int)bt.size());
            int index = uniform_dist_s(e1);
            auto pos = bt.find_by_index(index);
            if (std::get<0>(pos)) {
                auto &node = std::get<1>(pos);
                auto &idx = std::get<2>(pos);
                auto val = node[idx];

                if (bt.exists(val)) {
                    series.push_back(-1 * val);
                    std::cout << "series(" << series.size() << "): " << series << '\n';
                    bt.remove(val);
                }
            }
        }

        // bt.dbg_dump();
        std::cout << '.';
        if (ix % 1000 == 0)
            std::cout << '\n'
                      << '\n'
                      << ix << ':' << '\n';
    }
    std::cout << "STOPPING..." << '\n';

    NO_ASSERTIONS_ONLY(bt.dbg_dump(std::cout));
    // dbg_dump<int, 3>(std::cout, bt);
}

int main(int argc, char *argv[]) {
    n_test_btree();
    int count = 0;
    if (argc > 1)
        count = std::atoi(argv[1]);
    n_test_btree_rand(count);
    std::cout << "END." << '\n';
    // test_btree_even();
    // test_btree_b();
}
