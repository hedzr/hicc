//
// Created by Hedzr Yeh on 2021/7/13.
//

#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>
#include <thread>

// 在 POSIX 系统上用 native_handle 启用 C++ 线程的实时调度

std::mutex iomutex;
void f(int num) {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    sched_param sch;
    int policy;
    pthread_getschedparam(pthread_self(), &policy, &sch);
    std::lock_guard<std::mutex> lk(iomutex);
    std::cout << "Thread " << num << " is executing at priority "
              << sch.sched_priority << '\n';
}

void test_native_handle() {
    std::thread t1(f, 1), t2(f, 2);

    sched_param sch;
    int policy;
    pthread_getschedparam(t1.native_handle(), &policy, &sch);
    sch.sched_priority = 20;
    if (pthread_setschedparam(t1.native_handle(), SCHED_FIFO, &sch)) {
        std::cout << "Failed to setschedparam: " << std::strerror(errno) << '\n';
    }

    t1.join();
    t2.join();
}