//
// Created by Hedzr Yeh on 2021/7/16.
//

#ifndef HICC_CXX_HZ_RINGBUF_HH
#define HICC_CXX_HZ_RINGBUF_HH

#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>

#include <functional>
#include <optional>
#include <queue>
#include <random>
#include <string>
#include <vector>

#include "hz-defs.hh"

namespace hicc::ringbuf {


    using cross::cacheline_align_t;
    using cross::cacheline_align_v;

    enum CannotEnqueue {
        CE_DEFAULT,
        CE_BLOCKED_AND_SPIN,
    };

    /**
     * @brief lock-free ring-buffer, MPMC enabled.
     * @tparam T 
     */
    template<typename T, int CANNOT_ENQUEUE = CE_DEFAULT>
    class ring_buffer {
    public:
        ring_buffer(int capacity = 2 << 8) {
            resize(capacity);
        }
        ~ring_buffer() { clear(); }

    public:
        void clear() {}
        // don't resize in working mode (enqueue/dequeue-ing).
        void resize(int capacity) {
            std::size_t size = _round_up_to_pow2(capacity);
            _coll.resize(size);
            _f = 0;
            _b = 0;
            _size = size;
            _Mask = size - 1;
        }

        // the free space
        std::size_t free() const {
            size_t b = _b.load(std::memory_order_acquire), f = _f.load(std::memory_order_acquire);
            return b > f ? b - f - 1 : _size + (b - f) - 1;
        }
        std::size_t qty() const {
            size_t b = _b.load(std::memory_order_acquire), f = _f.load(std::memory_order_acquire);
            return f > b ? f - b : _size + (f - b);
        }
        // the available capacity in this ring-buffer
        std::size_t capacity() const { return _Mask; }
        // the allocated size of the internal container
        std::size_t size() const { return _size; }
        bool empty() const {
            size_t b = _b.load(std::memory_order_acquire), f = _f.load(std::memory_order_acquire);
            return _empty(b, f);
        }
        bool full() const {
            size_t b = _b.load(std::memory_order_acquire), f = _f.load(std::memory_order_acquire);
            return _full(b, f);
        }
        bool _empty(size_t b, size_t f) const { return f == b; }
        inline bool _full(size_t b, size_t f) const { return ((f + 1) & _Mask) == b; }

    public:
        DISABLE_UNUSED_WARNINGS
        bool enqueue(T &&elem) {
            size_t b, f, nf;
        _retry:
            b = _b.load(std::memory_order_acquire), f = _f.load(std::memory_order_acquire);
            if (!_full(b, f)) {
                nf = (f + 1) & _Mask;
                if (_f.compare_exchange_strong(f, nf, std::memory_order_release)) {
                    _coll[f & _Mask] = std::move(elem);
                    return true;
                }
            }

            if constexpr (CANNOT_ENQUEUE == CE_DEFAULT)
                return false;
            if constexpr (CANNOT_ENQUEUE == CE_BLOCKED_AND_SPIN) {
                using namespace std::chrono_literals;
                std::this_thread::yield();
#if OS_LINUX
                std::this_thread::sleep_for(1ns);
#endif
                goto _retry;
            }
#if !OS_WIN
            return false;
#endif
        }
        std::optional<T> dequeue() {
            size_t b, f, nb;
            std::optional<T> ret;
        _retry:
            b = _b.load(std::memory_order_acquire), f = _f.load(std::memory_order_acquire);
            if (!_empty(b, f)) {
                nb = (b + 1) & _Mask;
                if (_b.compare_exchange_strong(b, nb, std::memory_order_release)) {
                    ret.emplace(std::move(_coll[b & _Mask]));
                    return ret;
                }
            }

            if constexpr (CANNOT_ENQUEUE == CE_DEFAULT)
                return ret; // std::nullopt;
            if constexpr (CANNOT_ENQUEUE == CE_BLOCKED_AND_SPIN) {
                using namespace std::chrono_literals;
                std::this_thread::yield();
#if OS_LINUX
                std::this_thread::sleep_for(1ns);
#endif
                goto _retry;
            }
#if !OS_WIN
            return ret;
#endif
        }
        RESTORE_UNUSED_WARNINGS

    private:
        template<typename TI>
        inline TI next_pow2(TI v) {
#if __GNUG__
            // optimized as: bsrq	-8(%rbp), %rax
            return v == 1 ? 1 : 1 << (64 - __builtin_clzl(v - 1));
#else
            TI p = 1;
            while (p < v) p <<= 1;
            return p;
#endif
            // About 'round up to nearest power of 2':
            // https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
            // https://jameshfisher.com/2018/03/30/round-up-power-2/
            // https://stackoverflow.com/questions/466204/rounding-up-to-next-power-of-2
        }
        inline std::size_t _round_up_to_pow2(std::size_t v) {
            // v--;
            // v |= v >> 1;
            // v |= v >> 2;
            // v |= v >> 4;
            // v |= v >> 8;
            // v |= v >> 16;
            // v |= v >> 32;
            // v++;
            return next_pow2(v);
        }

    private:
        std::vector<T> _coll;
        alignas(cacheline_align_v)
                std::atomic_size_t _f; // front ptr: the enqueue item will be put in here
        alignas(cacheline_align_v)
                std::atomic_size_t _b; // back ptr: the dequeue item will be pop from here
        alignas(cacheline_align_v)
                std::size_t _size,
                _Mask;
    }; // class ring_buffer


} // namespace hicc::ringbuf

#endif //HICC_CXX_HZ_RINGBUF_HH
