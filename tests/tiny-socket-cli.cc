//
// Created by Hedzr Yeh on 2021/8/31.
//

#include "hicc/hz-defs.hh"

#include "hicc/hz-dbg.hh"
#include "hicc/hz-pool.hh"
#include "hicc/hz-process.hh"

int port = 2333;

char buf[256];

#if !OS_WIN

#if defined(__clang__)
#pragma clang poison printf sprintf fprintf
#elif defined(__GNUC__) || defined(__GNUG__)
#pragma GCC poison printf sprintf fprintf
#endif
void test_func_ppoisoning() {
    // sprintf(buf, "hello");
}

namespace demo {

}

void test_cli() {
}

#else // Windows

// Windows:
void test_func_ppoisoning() {
    // sprintf(buf, "hello");
}

void test_cli() {
}

#endif

int main() {
    test_func_ppoisoning();
    test_cli();
    return 0;
}