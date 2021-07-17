//
// Created by Hedzr Yeh on 2021/7/15.
//

#include <future>
#include <mutex>
#include <pthread.h>
#include <random>
#include <thread>

#include <chrono>
#include <cstring>
#include <iostream>

#include <algorithm>
#include <deque>
#include <optional>
#include <sstream>
#include <utility>
#include <vector>

#include "hicc/hz-log.hh"

#include "hicc/hz-pool.hh"
#include "hicc/hz-ringbuf.hh"


namespace oliver {

    constexpr std::size_t cacheline_align_v = 64;

    struct VV {
        char a;
        alignas(cacheline_align_v)
                std::atomic_size_t _f;
        alignas(cacheline_align_v)
                std::atomic_size_t _b;
        alignas(cacheline_align_v)
                std::size_t _size,
                _Mask;
    } vv1 = {'a', 9, 8, 7, 6};
    // vv1:
    // .byte   97
    // .zero   63
    // .quad   9
    // .zero   56
    // .quad   8
    // .zero   56
    // .quad   7
    // .zero   56
    // .quad   6
    // .zero   56

    void test_vv1() {
        printf("%c, %lu\n", vv1.a, vv1._f.operator unsigned long());
    }

} // namespace oliver

typedef hicc::ringbuf::ring_buffer<std::string, hicc::ringbuf::CE_BLOCKED_AND_SPIN> blocked_ring_buf;

struct tiny_pool {
    std::mutex m;
    std::condition_variable cv;
    bool ready = false;

    class capture {
    public:
        capture(tiny_pool &tp)
            : _tp(tp)
            , _lk(tp.m) {}
        ~capture() {
            _tp.ready = true;
            std::notify_all_at_thread_exit(_tp.cv, std::move(_lk));
        }
        tiny_pool &_tp;
        std::unique_lock<std::mutex> _lk;
    };

    void wait() {
        std::unique_lock<std::mutex> lk(m);
        while (!ready) {
            cv.wait(lk);
        }
    }
}; // class tiny_pool

void test_tiny_pool() {
}

std::function<void()> foo(unsigned int OD, blocked_ring_buf &rb) {

    using namespace std::chrono_literals;
    std::default_random_engine engine{std::random_device{}()};
    std::uniform_int_distribution<int64_t> dist{10, 300};
    int64_t wait_time = dist(engine);

    if ((OD & 1) == 0) {
        return [&rb, wait_time, OD]() {
            hicc_debug("~ thread #%02d [enqueue]", OD);
            for (int i = 0; i < 16; i++) {
                std::stringstream ss;
                ss << "item " << OD * 64 + i;
                auto str = ss.str();
                auto str1 = str;
                if (rb.enqueue(std::move(str)))
                    hicc_debug("  - T%02d: put '%s', '%s' ok: rb.size=%u, rb.free=%u, rb.qty=%u", OD, str1.c_str(), str.c_str(), rb.capacity(), rb.free(), rb.qty());
                else
                    hicc_debug("  - T%02d: enqueue '%s', '%s' failed: rb.size=%u, rb.free=%u, rb.qty=%u", OD, str1.c_str(), str.c_str(), rb.capacity(), rb.free(), rb.qty());
                std::this_thread::yield();
                // std::this_thread::sleep_for(300ms);
                std::this_thread::sleep_for(std::chrono::milliseconds{wait_time});
            }
            UNUSED(OD);
        };
    }

    return [&rb, wait_time, OD]() {
        hicc_debug("~ thread #%02d [dequeue]", OD);
        for (int i = 0; i < 16; i++) {
            auto ret = rb.dequeue();
            if (ret.has_value()) {
                std::string &str = ret.value();
                hicc_debug("  - T%02d: got '%s' ok: rb.size=%u, rb.free=%u, rb.qty=%u", OD, str.c_str(), rb.capacity(), rb.free(), rb.qty());
                UNUSED(str);
            } else
                hicc_debug("  - T%02d: dequeue failed: rb.size=%u, rb.free=%u, rb.qty=%u", OD, rb.capacity(), rb.free(), rb.qty());
            std::this_thread::yield();
            // std::this_thread::sleep_for(300ms);
            std::this_thread::sleep_for(std::chrono::milliseconds{wait_time});
        }
        UNUSED(OD);
    };
}

void test_ringbuf() {
    {
        // simple test

        hicc::ringbuf::ring_buffer<std::string> rb(7);
        for (int i = 0; i < 16; i++) {
            std::stringstream ss;
            ss << "item " << i;
            auto str = ss.str();
            auto str1 = str;
            if (rb.enqueue(std::move(str)))
                hicc_debug("  - T%02d: put '%s', '%s' ok: rb.size=%u, rb.free=%u, rb.qty=%u", i, str1.c_str(), str.c_str(), rb.capacity(), rb.free(), rb.qty());
            else
                hicc_debug("  - T%02d: enqueue '%s', '%s' failed: rb.size=%u, rb.free=%u, rb.qty=%u", i, str1.c_str(), str.c_str(), rb.capacity(), rb.free(), rb.qty());
        }
        for (int i = 0; i < 16; i++) {
            auto ret = rb.dequeue();
            if (ret.has_value()) {
                std::string &str = ret.value();
                hicc_debug("  - T%02d: got '%s' ok: rb.size=%u, rb.free=%u, rb.qty=%u", i, str.c_str(), rb.capacity(), rb.free(), rb.qty());
                UNUSED(str);
            } else
                hicc_debug("  - T%02d: dequeue failed: rb.size=%u, rb.free=%u, rb.qty=%u", i, rb.capacity(), rb.free(), rb.qty());
        }
    }

    hicc_debug("");
    hicc_debug("");

    {
        // enqueue/dequeue test

        // tiny_pool tp1;
        hicc::pool::thread_pool threads(std::thread::hardware_concurrency());
        hicc::pool::threaded_message_queue<std::string> messages;
        blocked_ring_buf rb(7);
        for (unsigned int ti = 0; ti < std::thread::hardware_concurrency(); ti++) {
            // std::thread t(foo(ti, rb));
            // t.detach();
            threads.queue_task(foo(ti, rb));
        }
        hicc_debug("waiting...");
        threads.join();
    }
}

int main() {
    std::cout << "cache_line: " << hicc::cross::cache_line_size() << '\n';
    std::cout << "hardware_constructive_interference_size: " << hicc::cross::hardware_constructive_interference_size << '\n';
    std::cout << "hardware_destructive_interference_size: " << hicc::cross::hardware_destructive_interference_size << '\n';
    std::cout << "sizeof(std::max_align_t): " << sizeof(std::max_align_t) << '\n';

    // test_mq();

    test_tiny_pool();

    test_ringbuf();
}
