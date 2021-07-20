//
// Created by Hedzr Yeh on 2021/7/13.
//

#ifndef HICC_CXX_TICKER_HH
#define HICC_CXX_TICKER_HH

#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include <tuple>

#include "hz-defs.hh"

namespace hicc::chrono {

    /**
     * @brief a posix timer wrapper class
     * 
     * inspired by https://github.com/Bosma/Scheduler and https://github.com/jmettraux/rufus-scheduler.
     */
    class ticker {
    public:
        class posix_ticker {
        public:
        }; // class posix_ticker

    public:
        ticker() {}
        ~ticker() {}

        template <class _Rep, class _Period>
        ticker &every(const std::chrono::duration<_Rep, _Period>& __d) {
            UNUSED(__d);
            return (*this);
        }
        template <class _Rep, class _Period>
        ticker &in(const std::chrono::duration<_Rep, _Period>& __d) {
            UNUSED(__d);
            return (*this);
        }
        template <class _Rep, class _Period>
        ticker &at(const std::chrono::duration<_Rep, _Period>& __d) {
            UNUSED(__d);
            return (*this);
        }
        template <class _Rep, class _Period>
        ticker &cron(const std::chrono::duration<_Rep, _Period>& __d) {
            UNUSED(__d);
            return (*this);
        }
    }; // class ticker

} // namespace hicc::chrono

#endif //HICC_CXX_TICKER_HH
