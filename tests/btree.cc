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


template<class T, int Degree = 5>
void dbg_dump(std::ostream &os, hicc::btree::btree<T, Degree, std::less<T>, true> const &bt) {
    os << "DUMP...";
    bt.walk_level_traverse([&os](typename hicc::btree::btree<T, Degree, std::less<T>, true>::traversal_context const &ctx) -> bool {
        if (ctx.node_changed) {
            if (ctx.index == 0)
                os << '\n'
                   << ' ' << ' '; //  << 'L' << level << '.' << ptr_index;
            else
                os << ' ';
            // if (node._parent == nullptr) os << '*'; // root here
            os //<< ptr_index << '.'
                    << ctx.curr.to_string();
        }
        os.flush();
        return true; // false to terminate the walking
    });
    os << '\n';
}

void test_btree() {

    hicc::chrono::high_res_duration hrd([](auto duration) -> bool {
        std::cout << "It took " << duration << '\n';
        return false;
    });

    using btree = hicc::btree::btree<int, 5, std::less<int>, true>;
    btree bt;
    bt.dot_prefix("tree");
    bt.insert(9, 11, 2, 7);
    dbg_dump(std::cout, bt);
    bt.insert(3); // split
    dbg_dump(std::cout, bt);
    bt.insert(5);
    bt.insert(13, 17);
    dbg_dump(std::cout, bt);
    bt.insert(19);
    bt.dbg_dump(); // split

    bt.remove(19);
    bt.dbg_dump();
    bt.insert(19);
    bt.dbg_dump(); // split

    bt.insert(21, 6, 8, 12, 29);
    bt.dbg_dump();
    bt.insert(31);
    bt.dbg_dump();
    bt.insert(18, 20);
    bt.dbg_dump();
    bt.insert(16); // split
    bt.dbg_dump();
    bt.insert(14, 15);
    bt.dbg_dump();

    bt.remove(15);
    bt.dbg_dump();
    bt.insert(15);
    bt.dbg_dump(); // split

    bt.insert(1); // split
    bt.dbg_dump();

    bt.remove(1);
    bt.dbg_dump();
    bt.insert(1);
    bt.dbg_dump(); // split

    bt.insert(10);
    bt.dbg_dump();
    bt.insert(33, 47);
    bt.dbg_dump();
    bt.insert(28);
    bt.dbg_dump();
    bt.insert(55, 56, 66, 78);
    bt.dbg_dump();
    bt.insert(87);
    bt.dbg_dump();
    bt.insert(69);
    bt.dbg_dump();

    bt.remove(69);
    bt.dbg_dump();
    bt.insert(69);
    bt.dbg_dump();
    // dbg_dump(std::cout, bt); // split

    // std::cout << "ITER..." << '\n';
    // for (auto &it : pq) {
    //     std::cout << it << '\n';
    // }
    // std::cout << "POP..." << '\n';
    // while (!pq.empty())
    //     std::cout << pq.pop() << '\n';
}

void test_btree_1_emplace() {
    std::string str(5, 's');
    hicc::btree::btree<std::string> bts;
    bts.emplace(5, 's');
}

hicc::btree::btree<int, 5, std::less<int>, true> test_btree_1() {
    using hicc::terminal::colors::colorize;
    colorize c;
    std::cout << c.bold().s("test_btree_1") << '\n';
    hicc::chrono::high_res_duration hrd([](auto duration) -> bool {
        std::cout << "It took " << duration << '\n';
        return false;
    });

    using btree = hicc::btree::btree<int, 5, std::less<int>, true>;
    btree bt;
    bt.dot_prefix("tr01");
    bt.insert(39, 22, 97, 41);
    bt.dbg_dump();
    bt.insert(53);
    bt.dbg_dump();
    bt.insert(13, 21, 40);
    bt.dbg_dump();
    bt.insert(30, 27, 33);
    bt.dbg_dump();
    bt.insert(36, 35, 34);
    bt.dbg_dump();
    bt.insert(24, 29);
    bt.dbg_dump();
    bt.insert(26);
    bt.dbg_dump();
    bt.insert(17, 28, 23);
    bt.dbg_dump();
    bt.insert(31, 32);
    bt.dbg_dump();

    std::cout << '\n';
    int count = 0;
    bt.walk([&count](btree::traversal_context const &ctx) -> bool {
#ifdef _DEBUG
        static int last_one = 0;
        assertm(last_one <= *ctx.el, "expecting a ascend sequences");
        last_one = *ctx.el;
#endif
        std::cout << ctx.el << ',';
        count++;
        return true;
    });
    std::cout << '\n';
    std::cout << "total: " << count << " elements" << '\n';

    return bt;
}

void test_btree_1_remove(hicc::btree::btree<int, 5, std::less<int>, true> &bt) {
    if (auto [idx, ref] = bt.next_payload(bt.find(21)); ref) {
        assert(ref[idx] == 22);
    } else {
        assert(false && "'21' not found");
    }
    if (auto [idx, ref] = bt.next_minimal_payload(bt.find(32)); ref) {
        // std::cout << ref[idx] << '\n';
        assert(ref[idx] == 33);
    } else {
        assert(false && "'32' not found");
    }

    auto vec = bt.to_vector();

    for (auto it = vec.begin(); it != vec.end(); it++) {
        auto val = (*it);
        if (auto [ok, rr, ii] = bt.find(val); ok) {
            if (auto [idx, ref] = bt.next_minimal_payload(rr, ii); ref) {
                std::cout << ref[idx] << " >> " << val << '\n';

                auto tmp = it;
                tmp++;
#ifdef _DEBUG
                auto next_val = (*tmp);
                assert(ref[idx] == next_val);
                UNUSED(next_val);
#endif
            } else if (val != vec.back()) {
                std::ostringstream msg;
                msg << '"' << val << '"' << " next_minimal_payload failed.";
                assertm(false, msg.str().c_str());
            }
        } else {
            std::ostringstream msg;
            msg << '"' << val << '"' << " cannot be found.";
            assertm(false, msg.str().c_str());
        }
    }

    bt.remove(21);
    bt.dbg_dump();
    bt.remove(17);
    bt.dbg_dump();
    bt.remove(22);
    bt.dbg_dump();
    bt.remove(13, 23);
    bt.dbg_dump();
    bt.remove(24);
    bt.dbg_dump();
    bt.remove(26);
    bt.dbg_dump();
    bt.remove(27);
    bt.dbg_dump();
    bt.remove(28);
    bt.dbg_dump();

    for (auto v : vec) {
        if (v == 97) {
            // bt.dbg_dump();
            printf("");
        }
        bt.remove(v);
        bt.dbg_dump();
    }
}

void test_btree_2() {
    using hicc::terminal::colors::colorize;
    colorize c;
    std::cout << c.bold().s("test_btree_2") << '\n';

    hicc::chrono::high_res_duration hrd([](auto duration) -> bool {
        std::cout << "It took " << duration << '\n';
        return false;
    });

    using btree = hicc::btree::btree<int, 3, std::less<int>, false>;
    btree bt;
    bt.dot_prefix("tr02");
    // bt.insert(50, 128, 168, 140, 145, 270, 250, 120, 105, 117, 264, 269, 320, 439, 300, 290, 226, 155, 100, 48, 79);
    bt.insert(50, 128, 168, 140, 145, 270);
    bt.dbg_dump();
    bt.insert(250);
    bt.dbg_dump();

    // bt.remove(250);
    // dbg_dump<int, 3>(std::cout, bt);
    // bt.insert(250);
    // dbg_dump<int, 3>(std::cout, bt);

    bt.insert(120, 105, 117, 264);
    bt.dbg_dump();
    bt.insert(269, 320, 439, 300);
    bt.dbg_dump();
    bt.insert(290, 226, 155, 100);
    bt.dbg_dump();
    bt.insert(48);
    bt.dbg_dump();
    bt.insert(79);
    bt.dbg_dump();

    // bt.dot("bb2.1.dot");

    // bt.remove(79);
    // bt.dbg_dump();
    // //bt.remove(250);
    // //bt.dbg_dump();
    // // bt.dot("bb2.2.dot");

    auto l = [&bt](int k) {
        if (auto [ok, ref, idx] = bt.find(k); ok) {
            std::cout << "searching '" << k << "' found: " << ref.to_string() << ", idx = " << idx << '\n';
        } else {
            std::cerr << "searching '" << k << "': not found\n";
        }
    };
    l(270);
    l(226);
    l(100);

    for (auto &v : bt.to_vector()) {
        std::cout << v << ',';
    }
    std::cout << '\n';

    bt.walk([](btree::traversal_context const &ctx) -> bool {
        std::cout << std::boolalpha << *ctx.el << "/L" << ctx.level << "/IDX:" << ctx.abs_index << '/' << ctx.index
                  << "/PI:" << ctx.parent_ptr_index << "/" << ctx.parent_ptr_index_changed << "/" << ctx.loop_base << '\n';
        return true;
    });
    std::cout << '\n';
    std::cout << "NLR:" << '\n';
    bt.walk_nlr([](btree::traversal_context const &ctx) -> bool {
        std::cout << std::boolalpha << *ctx.el << "/L" << ctx.level << "/IDX:" << ctx.abs_index << '/' << ctx.index
                  << "/PI:" << ctx.parent_ptr_index << "/" << ctx.parent_ptr_index_changed << "/" << ctx.loop_base << '\n';
        return true;
    });

    auto l2 = [&bt](int index) {
        if (auto [ok, ref, idx] = bt.find_by_index(index); ok) {
            std::cout << "searching index '" << index << "' found: " << ref.to_string() << ", idx = " << idx << '\n';
        } else {
            std::cerr << "searching index '" << index << "': not found\n";
        }
    };
    l2(0);
    l2(1);
    l2(2);
    l2(3);
}

void test_btree_b() {
    hicc::chrono::high_res_duration hrd;

    int ix{};
    using btree = hicc::btree::btree<int, 3, std::less<int>, true>;
    btree bt;
    bt.dot_prefix("tr.b");
    // auto numbers = {12488, 2222, 26112, -1, -1, 13291, 23251, 32042, 19569};
    // auto numbers = {1607, 4038, 1100, 2937, 3657, 10151};
    // auto numbers = {15345, 5992, 21742, 3387, 9020, 18896, 27523, 28967, -1};
    // auto numbers = {21140, 16213, 3732, 27322, 27594, -2, -1, -1, 7347, -1, 5134, -1, 19740,28912, 17131};
    // auto numbers = {7789, 20579, 1622, 153, 1449, 12733, 9056, 11643, 25126, 18723, 17262, 23071, 31250, 31575, -10};
    // [7789,20579]/11
    //              [1622]/9 [12733]/0 [25126]/0
    //              [153,1449]/17 [5564,8669]/0 | [9056,11643]/0 [18723]/0 | [17262,23071]/0 [31350,31575]/0
    // auto numbers = {9004, 13782, 3060, 11383, 15034, 15737, -1};
    // [15315]/0
    //         [14925]/9 [29854,31496]/0
#if 0
    auto numbers = {
            -1,
            -1,
            -1,
            -1,
            -1,
            25356,
            -2,
            29923,
            -1,
            7611,
            -1,
            28963,
            3610,
            -4,
            1825,
            -5,
            12286,
            -3,
            25236,
            5144,
            14000,
            -8,
            -4,
    };
    auto numbers = {
            19220,
            -2,
            -2,
            -2,
            24999,
            30209,
            -4,
            -4,
            15934,
            8618,
            -4,
            360,
            -4,
    };
#endif
    auto numbers = {
            8704,
            5834,
            -1,
            -1,
            13159,
            22419,
            4806,
            -4,
            31437,
            12326,
            -5,
            4303,
            -3,
    };

    for (auto n : numbers) {
        if (n > 0) {
            if (ix == 5)
                ix += 0;
            bt.insert(n);
            bt.dbg_dump();
        } else {
            auto index = abs(1 + n);
            auto pos = bt.find_by_index(index);
            if (ix == 12)
                ix += 0;
            bt.remove_by_position(pos);
            bt.dbg_dump();
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

    using btree = hicc::btree::btree<int, 3, std::less<int>, true>;
    std::vector<int> series;
    btree bt;
    bt.dot_prefix("rand");
    for (int ix = 0; ix < 2000 * 1000; ix++) {
        // int choice = std::rand();
        int choice = uniform_dist(e1);
        if (choice > MAX / 2) {
            // int random_variable = std::rand();
            int random_variable = uniform_dist(e1);
            series.push_back(random_variable);
            for (auto vv : series) std::cout << vv << ',';
            std::cout << '\n'
                      << "cnt: " << series.size() << '\n';
            bt.insert(random_variable);
        } else {
            // int index1 = std::rand();
            // auto index = (int) (((double) index1) / RAND_MAX * bt.size());
            std::uniform_int_distribution<int> uniform_dist_s(0, bt.size());
            int index = uniform_dist_s(e1);
            auto pos = bt.find_by_index(index);
            series.push_back(-1 - index);
            for (auto vv : series) std::cout << vv << ',';
            std::cout << '\n'
                      << "cnt: " << series.size() << '\n';
            bt.remove_by_position(pos);
        }

        bt.dbg_dump();
        std::cout << '.';
        if (ix % 1000 == 0)
            std::cout << '\n'
                      << ix << '\n';
    }
    std::cout << '\n';

    dbg_dump<int, 3>(std::cout, bt);
}

template<class T, class _D = std::default_delete<T>>
class dxit {
public:
    dxit() {}
    ~dxit() {}

private:
};

int main() {
#if 1

#if 0
    // assertm(1 == 0, "bad");
    test_btree();

    auto bt = test_btree_1();
#if 0
    if (false) {
        using btree = decltype(bt);
        btree::size_type total = bt.size();
        {
            // auto ofs = std::make_unique<std::ofstream>("aa.dot", std::ofstream::out);
            std::unique_ptr<std::ofstream, std::function<void(std::ofstream *)>> ofs(new std::ofstream("aa.dot", std::ofstream::out), [](std::ofstream *) {
                std::cout << "aa.dot pre-closed.\n";
            });
            *ofs.get() << "1";
        }
        std::ofstream ofs("aa.dot", std::ofstream::out);
        hicc::util::defer ofs_closer(ofs, []() {
            std::cout << "aa.dot was written." << '\n';
            hicc::process::exec dot("dot aa.dot -T png -o aa.png");
            std::cout << dot.rdbuf();
        });
        hicc::util::defer<bool> a_closer([&total]() { total = 0; });
        bt.walk_level_traverse([total, &ofs](btree::traversal_context const &ctx) -> bool {
            if (ctx.abs_index == 0) {
                ofs << "graph {" << '\n';
            }
            if (ctx.node_changed) {
                auto from = std::quoted(ctx.curr.to_string());
                if (ctx.abs_index == 0) {
                    ofs << from << " [color=green];" << '\n';
                }
                for (int i = 0; i < ctx.curr.degree(); ++i) {
                    if (auto const *child = ctx.curr.child(i); child) {
                        ofs << from << " -- ";
                        ofs << std::quoted(child->to_string()) << ';'
                            << ' ' << '#' << ' ' << std::boolalpha
                            << ctx.abs_index << ':' << ' '
                            << ctx.level << ',' << ctx.index << ',' << ctx.parent_ptr_index
                            << ',' << ctx.loop_base << ','
                            << ctx.level_changed
                            << ',' << ctx.parent_ptr_index_changed
                            << '\n';
                    } else
                        break;
                }
                ofs << '\n';
            }
            if (ctx.abs_index == (int) total - 1) {
                ofs << "}" << '\n';
            }
            // std::cout << "abs_idx: " << ctx.abs_index << ", total: " << total << '\n';
            return true;
        });
    }
#endif
    // bt.dot("aa.dot");
    test_btree_1_remove(bt);
#endif

    test_btree_2();

#elif 1
    test_btree_b();
#else
    test_btree_rand();
#endif
}
