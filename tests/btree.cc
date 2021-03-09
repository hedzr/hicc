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
#include <random>


template<class T, int Degree = 5>
void dbg_dump(std::ostream &os, hicc::btree::btree<T, Degree> const &bt) {
    os << "DUMP...";
    bt.walk_level_traverse([&os](
                                   typename hicc::btree::btree<T, Degree>::const_elem_ref el,
                                   typename hicc::btree::btree<T, Degree>::const_node_ref node,
                                   int level, bool node_changed,
                                   int parent_ptr_index, bool parent_ptr_changed,
                                   int ptr_index) -> bool {
        if (node_changed) {
            if (ptr_index == 0)
                os << '\n'
                   << ' ' << ' '; //  << 'L' << level << '.' << ptr_index;
            else if (parent_ptr_changed)
                os << ' ' << '|' << ' ';
            else
                os << ' ';
            // if (node._parent == nullptr) os << '*'; // root here
            os //<< ptr_index << '.'
                    << node.to_string();
        }
        os.flush();
        UNUSED(el, node, level, node_changed, parent_ptr_index, parent_ptr_changed, ptr_index);
        return true; // false to terminate the walking
    });
    os << '\n';
}

void test_btree() {

    hicc::chrono::high_res_duration hrd([](auto duration) -> bool {
        std::cout << "It took " << duration << '\n';
        return false;
    });

    hicc::btree::btree<int> bt;
    bt.insert(9, 11, 2, 7);
    dbg_dump(std::cout, bt);
    bt.insert(3); // split
    dbg_dump(std::cout, bt);
    bt.insert(5);
    bt.insert(13, 17);
    dbg_dump(std::cout, bt);
    bt.insert(19);
    dbg_dump(std::cout, bt); // split

    bt.remove(19);
    dbg_dump(std::cout, bt);
    bt.insert(19);
    dbg_dump(std::cout, bt); // split

    bt.insert(21, 6, 8, 12, 29);
    dbg_dump(std::cout, bt);
    bt.insert(31);
    dbg_dump(std::cout, bt);
    bt.insert(18, 20);
    dbg_dump(std::cout, bt);
    bt.insert(16); // split
    dbg_dump(std::cout, bt);
    bt.insert(14, 15);
    dbg_dump(std::cout, bt);

    bt.remove(15);
    dbg_dump(std::cout, bt);
    bt.insert(15);
    dbg_dump(std::cout, bt); // split

    bt.insert(1); // split
    dbg_dump(std::cout, bt);

    bt.remove(1);
    dbg_dump(std::cout, bt);
    bt.insert(1);
    dbg_dump(std::cout, bt); // split

    bt.insert(10);
    dbg_dump(std::cout, bt);
    bt.insert(33, 47);
    dbg_dump(std::cout, bt);
    bt.insert(28);
    dbg_dump(std::cout, bt);
    bt.insert(55, 56, 66, 78);
    dbg_dump(std::cout, bt);
    bt.insert(87);
    dbg_dump(std::cout, bt);
    bt.insert(69);
    dbg_dump(std::cout, bt);

    bt.remove(69);
    dbg_dump(std::cout, bt);
    bt.insert(69);
    dbg_dump(std::cout, bt); // split

    // std::cout << "ITER..." << '\n';
    // for (auto &it : pq) {
    //     std::cout << it << '\n';
    // }
    // std::cout << "POP..." << '\n';
    // while (!pq.empty())
    //     std::cout << pq.pop() << '\n';
}

void test_btree_1() {
    std::string str(5, 's');
    hicc::btree::btree<std::string> bts;
    bts.emplace(5, 's');
}

void test_btree_2() {
    using hicc::terminal::colors::colorize;
    colorize c;
    std::cout << c.bold().s("test_btree_2") << '\n';

    hicc::chrono::high_res_duration hrd([](auto duration) -> bool {
        std::cout << "It took " << duration << '\n';
        return false;
    });

    hicc::btree::btree<int, 3> bt;
    // bt.insert(50, 128, 168, 140, 145, 270, 250, 120, 105, 117, 264, 269, 320, 439, 300, 290, 226, 155, 100, 48, 79);
    bt.insert(50, 128, 168, 140, 145, 270);
    dbg_dump<int, 3>(std::cout, bt);
    bt.insert(250);
    dbg_dump<int, 3>(std::cout, bt);

    bt.remove(250);
    dbg_dump<int, 3>(std::cout, bt);

    bt.insert(250);
    dbg_dump<int, 3>(std::cout, bt);
    bt.insert(120, 105, 117, 264);
    dbg_dump<int, 3>(std::cout, bt);
    bt.insert(269, 320, 439, 300);
    dbg_dump<int, 3>(std::cout, bt);
    bt.insert(290, 226, 155, 100);
    dbg_dump<int, 3>(std::cout, bt);
    bt.insert(48);
    dbg_dump<int, 3>(std::cout, bt);
    bt.insert(79);
    dbg_dump<int, 3>(std::cout, bt);

    bt.remove(79);
    dbg_dump<int, 3>(std::cout, bt);
}

void test_btree_b() {
    hicc::chrono::high_res_duration hrd;

    int ix{};
    hicc::btree::btree<int, 3> bt;
    // auto numbers = {12488, 2222, 26112, -1, -1, 13291, 23251, 32042, 19569};
    // auto numbers = {1607, 4038, 1100, 2937, 3657, 10151};
    // auto numbers = {15345, 5992, 21742, 3387, 9020, 18896, 27523, 28967, -1};
    // auto numbers = {21140, 16213, 3732, 27322, 27594, -2, -1, -1, 7347, -1, 5134, -1, 19740,28912, 17131};
    // auto numbers = {7789, 20579, 1622, 153, 1449, 12733, 9056, 11643, 25126, 18723, 17262, 23071, 31250, 31575, -10};
    // [7789,20579]/11
    //              [1622]/9 [12733]/0 [25126]/0
    //              [153,1449]/17 [5564,8669]/0 | [9056,11643]/0 [18723]/0 | [17262,23071]/0 [31350,31575]/0
    auto numbers = {9004,13782,3060,11383,15034,15737, -1};
    // [15315]/0
    //         [14925]/9 [29854,31496]/0
    
    for (auto n : numbers) {
        if (n > 0) {
            if (ix == 5) {
                ix++;
            }
            bt.insert(n);
            bt.dbg_dump(std::cout);
        } else {
            auto index = abs(1 + n);
            auto pos = bt.find_by_index(index);
            bt.remove_by_position(pos);
            bt.dbg_dump(std::cout);
        }
        ix++;
    }
}

void test_btree_rand() {
    hicc::chrono::high_res_duration hrd;

    std::cout << '\n'
              << "random test..." << '\n'
              << '\n';

    const int MAX = 32768;
    std::random_device r;
    std::default_random_engine e1(r());
    std::uniform_int_distribution<int> uniform_dist(1, MAX);

    std::srand((unsigned int) std::time(nullptr)); // use current time as seed for random generator

    hicc::btree::btree<int, 3> bt;
    for (int ix = 0; ix < 2000 * 1000; ix++) {
        // int choice = std::rand();
        int choice = uniform_dist(e1);
        if (choice > MAX / 2) {
            // int random_variable = std::rand();
            int random_variable = uniform_dist(e1);
            bt.insert(random_variable);
        } else {
            // int index1 = std::rand();
            // auto index = (int) (((double) index1) / RAND_MAX * bt.size());
            std::uniform_int_distribution<int> uniform_dist_s(0, bt.size());
            int index = uniform_dist_s(e1);
            auto pos = bt.find_by_index(index);
            bt.remove_by_position(pos);
        }

        bt.dbg_dump(std::cout);
        std::cout << '.';
        if (ix % 1000 == 0)
            std::cout << '\n'
                      << ix << '\n';
    }
    std::cout << '\n';

    dbg_dump<int, 3>(std::cout, bt);
}

int main() {
    // // assertm(1 == 0, "bad");
    // test_btree();
    // // test_btree_1();
    // test_btree_2();

    test_btree_b();
    test_btree_rand();
}
