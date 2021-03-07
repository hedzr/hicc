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


template<class T, int Order = 5>
void dbg_dump(std::ostream &os, hicc::btree::btree<T, Order> const &bt) {
    os << "DUMP...";
    bt.walk_nlr([&os](
                        typename hicc::btree::btree<T, Order>::const_elem_ref el,
                        typename hicc::btree::btree<T, Order>::const_node_ref node,
                        int level, bool node_changed,
                        int parent_ptr_index, bool parent_ptr_changed,
                        int ptr_index) -> bool {
        if (node_changed) {
            if (ptr_index == 0)
                os << '\n'
                   << ' ' << ' '; // << 'L' << level << '.' << ptr_index;
            else if (parent_ptr_changed)
                os << ' ' << '|' << ' ';
            else
                os << ' ';
            // if (node._parent == nullptr) os << '*'; // root here
            os << node.to_string();
        }
        os.flush();
        UNUSED(el, level, parent_ptr_index, parent_ptr_changed, ptr_index);
        return true;
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
    bt.insert(21, 6, 8, 12, 29);
    dbg_dump(std::cout, bt);
    bt.insert(31);
    dbg_dump(std::cout, bt);
    bt.insert(18, 20);
    dbg_dump(std::cout, bt);
    bt.insert(16); // split
    dbg_dump(std::cout, bt);
    bt.insert(14, 15); // split
    dbg_dump(std::cout, bt);
    bt.insert(1); // split
    dbg_dump(std::cout, bt);
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
    bt.insert(120, 105, 117, 264);
    dbg_dump<int, 3>(std::cout, bt);
    bt.insert(269, 320, 439, 300, 290, 226, 155, 100, 48);
    dbg_dump<int, 3>(std::cout, bt);
    bt.insert(79);
    dbg_dump<int, 3>(std::cout, bt);
}

void test_btree_rand() {
    hicc::chrono::high_res_duration hrd;

    hicc::btree::btree<int, 3> bt;
    std::srand((unsigned int) std::time(nullptr)); // use current time as seed for random generator
    for (int ix = 0; ix < 2000 * 1000; ix++) {
        int random_variable = std::rand();
        bt.insert(random_variable);
    }

    dbg_dump<int, 3>(std::cout, bt);
}

int main() {
    assertm(1 == 0, "bad");
    test_btree();
    test_btree_2();
    test_btree_1();
}
