//
// Created by Hedzr Yeh on 2021/7/13.
//

#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>
#include <pthread.h>
#include <thread>

#include <sstream>

#include "hicc/hz-log.hh"
#include "hicc/hz-pool.hh"
#include "hicc/hz-x-class.hh"

void test_mq() {
    hicc::pool::threaded_message_queue<hicc::debug::X> xmq;

    hicc::debug::X x1("aa");
    {
        xmq.emplace_back(std::move(x1));
        hicc_debug("  xmq.emplace_back(std::move(x1)) DONE. AND, xmq.pop_front() ...");
        std::optional<hicc::debug::X> vv = xmq.pop_front();
        hicc_debug("vv (%p): '%s'", (void *) &vv, vv.value().c_str());
    }
    hicc_debug("x1 (%p): '%s'", (void *) &x1, x1.c_str());
}

void foo() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void test_thread() {
    {
        std::thread t;
        std::cout << "before starting, joinable: " << std::boolalpha << t.joinable()
                  << '\n';

        t = std::thread(foo);
        std::cout << "after starting, joinable: " << t.joinable() << ", id: " << t.get_id()
                  << '\n';

        t.join();
        std::cout << "after joining, joinable: " << t.joinable()
                  << '\n';
    }
}

void test_pool() {
    hicc::pool::thread_pool threads(10);
    hicc::pool::threaded_message_queue<std::string> messages;

    constexpr std::size_t message_count = 100;
    for (std::size_t i = 0; i < message_count; ++i) {
        threads.queue_task([i, &messages] {
            std::stringstream ss;
            ss << "Thread " << i << " completed";
            using namespace std::literals;
            // std::this_thread::sleep_for(i * 1ms);
            messages.push_back(ss.str());
        });
    }
    std::cout << "_tasks queued\n";
    std::size_t messages_read = 0;
    while (auto msg = messages.pop_front()) {
        std::cout << *msg << std::endl;
        ++messages_read;
        if (messages_read == message_count)
            break;
    }
}

int main() {
    // {
    //     unsigned int n = std::thread::hardware_concurrency();
    //     std::cout << n << " concurrent _threads are supported.\n";
    // }
    //
    // extern void test_native_handle();
    // test_native_handle();
    //
    // test_thread();
    //
    // test_pool();

    test_mq();
}