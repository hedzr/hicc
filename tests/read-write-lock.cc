//
// Created by Hedzr Yeh on 2021/7/21.
//
#include "hicc/hz-pool.hh"
#include <shared_mutex>

namespace test {

    typedef std::shared_mutex Lock;
    typedef std::unique_lock<Lock> WriteLock;
    typedef std::shared_lock<Lock> ReadLock;

    Lock l;
    int locked_counter;

    const int READER_MAX = 200;
    const int WRITER_MAX = 300;
    
    void reader(int i) {
        int c = 0;
        while (c++ < READER_MAX) {
            {
                ReadLock rl(l);
                printf("    #%d: %d\n", i, locked_counter);
            }

            using namespace std::literals::chrono_literals;
            // std::this_thread::yield();
            std::this_thread::sleep_for(10ms);
        }
        printf("    #%d: end of reader, %d\n", i, locked_counter);
    }

    void writer() {
        int c = 0;
        while (c++ < WRITER_MAX) {
            {
                WriteLock wl(l);
                //Do writer stuff
                locked_counter++;
            }

            // printf("[W]: %d\n", locked_counter);
            using namespace std::literals::chrono_literals;
            // std::this_thread::yield();
            std::this_thread::sleep_for(3ms);
        }
        printf("[W] end of writer: %d\n", locked_counter);
    }

} // namespace test

int main() {
    hicc::pool::thread_pool pool(5);
    for (int i = 0; i < 5; i++) {
        pool.queue_task([i]() {
            test::reader(i);
        });
    }

    std::thread t1(test::writer);
    t1.join();
    pool.join();
}