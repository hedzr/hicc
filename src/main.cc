#include <iostream>

#include "../include/core11.hh"

#include "hicc/hicc.hh"


void fatal_exit(const std::string &msg) {
    std::cerr << msg << '\n';
    exit(-1);
}

int main() {
    std::string s1("a string", 2, 3);
    std::cout << "Hello, World!" << s1 << '\n';

    hicc_debug("ok");
    
    return 0;
}
