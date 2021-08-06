//
// Created by Hedzr Yeh on 2021/3/1.
//

#ifndef HICC_CXX_HICC_HH
#define HICC_CXX_HICC_HH

#pragma once

#include "hicc-version.hh"

#include "hz-defs.hh"

#include "hz-if.hh"

#include "hz-chrono.hh"
#include "hz-ios.hh"
#include "hz-path.hh"
#include "hz-string.hh"

#include "hz-terminal.hh"

#include "hz-dbg.hh"
#include "hz-log.hh"

#include "hz-btree.hh"
#include "hz-os-io-redirect.hh"
#include "hz-priority-queue.hh"
#include "hz-process.hh"

#include "hz-mmap.hh"
#include "hz-ringbuf.hh"
#include "hz-pool.hh"

#include "hz-util.hh"

// #include "hz-x-class.hh"

namespace hicc {
    //
} // namespace hicc


#if defined(__cpp_exceptions)
#include "hz-if.hh"
#endif

#if defined(HICC_CXX_UNIT_TEST) && HICC_CXX_UNIT_TEST == 1
#include "hz-x-test.hh"
#endif


#endif //HICC_CXX_HICC_HH
