//
// Created by Hedzr Yeh on 2021/7/15.
//

#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>
// #include <pthread.h>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// #include "hicc/hz-pool.hh"
#include "hicc/hz-x-class.hh"

std::vector<std::string> vec;

void ff(std::string t) {
    vec.emplace_back(t);
    printf("  - vec[0]: %p, '%s'\n", (void *) vec.back().c_str(), vec.back().c_str());
}

void test_ff_1() {
    printf("\n");

    std::string str1("aa");
    printf("  = str1: %p, '%s'\n", (void *) str1.c_str(), str1.c_str());

    printf("\n");

    ff(str1);
    printf("  = str1: %p, '%s'\n", (void *) str1.c_str(), str1.c_str());
}

void test_ff_2() {
    printf("\n");
    ff("cc");
    ff("bb");
    ff("aa");
}

void test_ff() {
    test_ff_1();
    test_ff_2();
    printf("\n");
}

void fz(hicc::debug::X x) {
    printf("\n");

    hicc::debug::X v = std::move(x);
    printf("  = fz : v = [%p, '%s'], x = [%p, '%s']\n", (void *) &v, v.c_str(), (void *) &x, x.c_str());
}

void fz1(hicc::debug::X &&x) {
    printf("\n");

    hicc::debug::X v = std::move(x);
    printf("  = fz1: v = [%p, '%s'], x = [%p, '%s']\n", (void *) &v, v.c_str(), (void *) &x, x.c_str());
}

void test_fz() {
    hicc::debug::X x1("aa");
    fz(x1);
    fz1(std::move(x1));
}


int main() {
    test_ff();
    test_fz();
}
