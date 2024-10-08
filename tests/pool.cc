//
// Created by Hedzr Yeh on 2021/7/13.
//

#define HICC_TEST_THREAD_POOL_DBGOUT 1
#define HICC_ENABLE_THREAD_POOL_READY_SIGNAL 1

#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>
#include <thread>

#include <sstream>

#include "hicc/hz-log.hh"
#include "hicc/hz-pool.hh"
#include "hicc/hz-x-class.hh"
#include "hicc/hz-x-test.hh"

hicc::debug::X x_global_var;

class L {
public:
    L() {}
    virtual ~L() {}
    void lock() {}
    void unlock() {}
};

void test_basic_lockable() {
    {
        L l;
        std::lock_guard<L> lk(l);
        std::cout << x_global_var.c_str() << '\n';
    }
}

struct InkPen {
    void Write() {
        this->WriteImplementation();
    }

    void WriteImplementation() {
        std::cout << "Writing using a inkpen" << '\n';
    }
};

struct BoldPen {
    void Write() {
        std::cout << "Writing using a boldpen" << '\n';
    }
};

template<class PenPolicy>
class Writer : private PenPolicy {
public:
    void StartWriting() {
        PenPolicy::Write();
    }
};

void test_policy_1() {
    Writer<InkPen> writer;
    writer.StartWriting();
    Writer<BoldPen> writer1;
    writer1.StartWriting();
}

std::condition_variable cv100;
std::mutex cv100_m;
int count{0};

void test_cv_0() {
    std::vector<std::thread> waits;
    for (int i = 0; i < 100; i++) {
        auto t = std::thread([]() {
            {
                {
                    std::unique_lock<std::mutex> lk(cv100_m);
                    count++;
                }
                cv100.notify_one();

                // one thread running now
            }

            using namespace std::literals::chrono_literals;
            std::this_thread::yield();
            std::this_thread::sleep_for(10s);
            std::cout << "end of thread" << '\n';
        });
        waits.push_back(std::move(t));
    }

    {
        std::unique_lock<std::mutex> lk(cv100_m);
        cv100.wait(lk, []() { return count >= 100; });
    }

    // all threads ready, do stuff ...

    for (auto it = waits.begin(); it != waits.end(); it++) {
        auto &t = (*it);
        t.join();
    }
}

std::condition_variable cvpc;
std::mutex cvpc_m;
std::deque<int> cvpc_queue;
int seq;

void test_cv_1() {
    std::vector<std::thread> waits;
    for (int i = 0; i < 5; i++) {
        auto t = std::thread([]() {
            for (int j = 0; j < 10; j++) {
                {
                    std::unique_lock<std::mutex> lk(cvpc_m);
                    seq++;
                    cvpc_queue.push_back(seq);
                }
                cvpc.notify_one();

                // std::this_thread::yield();
            }
            // printf("#%d: ended.\n", i);
        });
        waits.push_back(std::move(t));
    }

    for (int i = 5; i < 15; i++) {
        auto t = std::thread([i]() {
            while (true) {
                {
                    std::unique_lock<std::mutex> lk(cvpc_m);
                    if (seq >= 50 && cvpc_queue.empty()) break;
                    cv100.wait(lk, []() { return !cvpc_queue.empty(); });
                }
                auto vv = cvpc_queue.front();
                cvpc_queue.pop_front();
                printf("#%d: got %d\n", i, vv);

                std::this_thread::yield();
            }
            // printf("#%d: ended.\n", i);
        });
        waits.push_back(std::move(t));
    }

    for (auto it = waits.begin(); it != waits.end(); it++) {
        auto &t = (*it);
        t.join();
    }
}

void test_cv() {
    {
        hicc::pool::conditional_wait_for_bool cv;
        auto t = std::thread([&cv]() {
            using namespace std::literals::chrono_literals;
            std::this_thread::yield();
            std::this_thread::sleep_for(1s);

            cv.set_for_all();
            std::cout << "end of thread" << '\n';
        });
        std::cout << "wait for cv" << '\n';
        cv.wait();
        std::cout << "end of cv test, join ..." << '\n';
        t.join();
        std::cout << "end of cv test" << '\n';
    }
    {
        std::vector<std::thread> waits;
        hicc::pool::conditional_wait_for_int cv(3);
        for (int i = 0; i < 3; i++) {
            auto t = std::thread([&cv]() {
                using namespace std::literals::chrono_literals;
                std::this_thread::sleep_for(2s);

                cv.set();
                std::cout << "end of thread" << '\n';
            });
            waits.push_back(std::move(t));
        }
        std::cout << "wait for cv" << '\n';
        cv.wait(); // waiting for all threads ended.

        std::cout << "end of cv test, join ..." << '\n';
        for (auto it = waits.begin(); it != waits.end(); it++) {
            auto &t = (*it);
            t.join();
        }
        std::cout << "end of cv test" << '\n';
    }
}

void test_mq() {
    hicc::pool::threaded_message_queue<hicc::debug::X> xmq;

    hicc::debug::X x1("aa");
    {
        xmq.emplace_back(std::move(x1));
        dbg_debug("  xmq.emplace_back(std::move(x1)) DONE. AND, xmq.pop_front() ...");
        std::optional<hicc::debug::X> vv = xmq.pop_front();
        dbg_debug("vv (%p): '%s'", (void *) &vv, vv.value().c_str());
    }
    dbg_debug("x1 (%p): '%s'", (void *) &x1, x1.c_str());
}

void foo() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void test_thread_basics() {
    std::thread t;
    std::cout << "- before starting, joinable: " << std::boolalpha << t.joinable()
              << '\n';

    t = std::thread(foo);
    std::cout << "- after starting, joinable: " << t.joinable() << ", id: " << t.get_id()
              << '\n';

    t.join();
    std::cout << "- after joining, joinable: " << t.joinable()
              << '\n';
}

void test_pool_x() {
    hicc::pool::thread_pool_lite pool(5);
    auto result = pool.enqueue([](int answer) { return answer; }, 42);
    std::cout << result.get() << std::endl;
}

void test_pool() {
    hicc::pool::thread_pool threads(5);
    hicc::pool::threaded_message_queue<std::string> messages;

    constexpr std::size_t message_count = 16;
    for (std::size_t i = 0; i < message_count; ++i) {
        threads.queue_task([i, &messages] {
            std::stringstream ss;
            ss << "Thread " << i << " completed";
            using namespace std::literals;
            // std::this_thread::sleep_for(i * 1ms);
            messages.emplace_back(ss.str());
            dbg_print("%s", ss.str().c_str());
            std::this_thread::yield();
        });
    }

    std::cout << "_tasks queued\n";
    std::size_t messages_read = 0;
    while (auto msg = messages.pop_front()) {
        std::cout << *msg << '\n';
        ++messages_read;
        if (messages_read == message_count)
            break;
    }

    threads.join();
    std::cout << "_tasks ended\n";
}

int main() {
    // {
    //     unsigned int n = std::thread::hardware_concurrency();
    //     std::cout << n << " concurrent _threads are supported.\n";
    // }

    // extern void test_native_handle();
    // test_native_handle();

    HICC_TEST_FOR(test_thread_basics);

    HICC_TEST_FOR(test_cv);

    HICC_TEST_FOR(test_cv_0);
    // HICC_TEST_FOR(test_cv_1);

    HICC_TEST_FOR(test_pool);

    HICC_TEST_FOR(test_mq);
}