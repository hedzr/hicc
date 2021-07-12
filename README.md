# hicc-cxx

Hi, cc! `hicc` is a C++17 template class library to provide some basic data structures and algorithms.

It's experimental, for studying basically. The stables of them will be migrated into [cmdr-cxx](https://github.com/hedzr/cmdr-cxx).




## For Developer



### Build

> gcc 10+: passed
>
> clang 12+: passed
>
> msvc build tool 16.7.2, 16.8.5 (VS2019 or Build Tool) passed

```bash
# configure
cmake -DENABLE_AUTOMATE_TESTS=OFF -S . -B build/
# build
cmake --build build/
# install
cmake --build build/ --target install
# sometimes maybe sudo: sudo cmake --build build/ --target install
```

### ninja, [Optional]

We used ninja for faster building.


### Other Options

1. `BUILD_DOCUMENTATION`=OFF
2. `ENABLE_TESTS`=OFF



## btree

The B-tree generic implementation here.

```cpp
#include "hicc/hz-btree.hh"
#include "hicc/hz-chrono.hh"

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
```







## Thanks to JODL

Thanks to [JetBrains](https://www.jetbrains.com/?from=hicc-cxx) for donating product licenses to help develop **hicc-cxx** [![jetbrains](https://gist.githubusercontent.com/hedzr/447849cb44138885e75fe46f1e35b4a0/raw/bedfe6923510405ade4c034c5c5085487532dee4/jetbrains-variant-4.svg)](https://www.jetbrains.com/?from=hedzr/hicc-cxx)




## LICENSE

MIT


## 🔚



