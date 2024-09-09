//
// Created by Hedzr Yeh on 2021/9/3.
//

#include "drawing.hh"

#include "hicc/hz-util.hh"
#include "hicc/hz-x-test.hh"

#include <iostream>
#include <math.h>
#include <string>

void test_drawing() {
    draw::drawing dr;
    dr.draw();
}

int main() {

    HICC_TEST_FOR(test_drawing);

    return 0;
}