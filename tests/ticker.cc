//
// Created by Hedzr Yeh on 2021/7/13.
//

#include "hicc/hz-ticker.hh"

#include <chrono>
#include <iostream>
#include <thread>

void foo() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void test_thread() {
    {
        std::thread t;
        std::cout << "before starting, joinable: " << std::boolalpha << t.joinable()
                  << '\n';

        t = std::thread(foo);
        std::cout << "after starting, joinable: " << t.joinable()
                  << '\n';

        t.join();
        std::cout << "after joining, joinable: " << t.joinable()
                  << '\n';
    }
    {
        unsigned int n = std::thread::hardware_concurrency();
        std::cout << n << " concurrent _threads are supported.\n";
    }
}

void test_ticker() {
}

int main() {
    test_thread();
    
    test_ticker();
    
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(5s);
    return 0;
}