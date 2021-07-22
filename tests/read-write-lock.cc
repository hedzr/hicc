//
// Created by Hedzr Yeh on 2021/7/21.
//
#include "hicc/hz-pool.hh"
#include <shared_mutex>

typedef std::shared_mutex Lock;
typedef std::unique_lock<Lock> WriteLock;
typedef std::shared_lock<Lock> ReadLock;

Lock l;
int locked_counter;

void reader(int i) {
    int c = 0;
    while (c++ < 200) {
        {
            ReadLock rl(l);
            printf("    #%d: %d\n", i, locked_counter);
        }

        using namespace std::literals::chrono_literals;
        // std::this_thread::yield();
        std::this_thread::sleep_for(30ms);
    }
    printf("    #%d: end of reader, %d\n", i, locked_counter);
}

void writer() {
    int c = 0;
    while (c++ < 500) {
        {
            WriteLock wl(l);
            //Do writer stuff
            locked_counter++;
        }

        // printf("[W]: %d\n", locked_counter);
        using namespace std::literals::chrono_literals;
        // std::this_thread::yield();
        std::this_thread::sleep_for(10ms);
    }
    printf("[W] end of writer: %d\n", locked_counter);
}

int main() {
    hicc::pool::thread_pool pool(5);
    for (int i = 0; i < 5; i++) {
        pool.queue_task([i]() {
            reader(i);
        });
    }

    std::thread t1(writer);
    t1.join();
    pool.join();
}