//
// Created by Hedzr Yeh on 2021/3/1.
//

#ifndef HICC_CXX_HZ_UTIL_HH
#define HICC_CXX_HZ_UTIL_HH

#include <any>
#include <optional>
#include <variant>

#include <list>
#include <map>
#include <vector>

#include <algorithm>
#include <functional>
#include <utility>

#include <cassert>
#include <stdexcept>

#include <iomanip>
#include <iostream>
#include <sstream>

#include <cstring>
#include <string>

#include <unordered_map>
#include <unordered_set>

#include <type_traits>
#include <typeindex>
#include <typeinfo>

#include <memory>

#include <chrono>

#ifndef __FS_COMPATIBLE
#define __FS_COMPATIBLE
#ifdef __cpp_lib_filesystem
#include <filesystem>
#else

#if __cplusplus < 201703L
#include <experimental/filesystem>
namespace std {
    namespace filesystem = experimental::filesystem;
}
#else
#include <filesystem>
#endif

#endif
#endif

#include "hz-common.hh"

#include "hz-chrono.hh"
#include "hz-dbg.hh"

namespace hicc::util {

#if 0
    template<typename T>
    class Nullable {
    public:
        Nullable() = default;

        virtual ~Nullable() {
            if (_value) delete _value;
        }

    public:
        Nullable(const Nullable &o) { _copy(o); }

        Nullable &operator=(const Nullable &o) {
            _copy(o);
            return *this;
        }

        Nullable &operator=(const T &o) {
            this->_value = o;
            return *this;
        }

    private:
        void _copy(const Nullable &o) {
            this->_value = o._value;
        }

    public:
        T &val() { return *_value; }

        const T &val() const { return *_value; }

        void val(T &&v) {
            if (!_value)
                _value = new T;
            (*_value) = v;
        }

        explicit operator T() const { return val(); }

        explicit operator T() { return val(); }

        // operator ->
        // operator *

        // operator ==, !=, >=, <=, >, <, <=>
        // operator +,-,*,/,...

        [[nodiscard]] bool is_null() const { return !_value; }

    private:
        T *_value{nullptr};
    }; // class Nullable<T>
#endif


#if 0
    template<class OS = std::ostream>
    class streamer_any {
    public:
        // OS &os;
        typedef std::unordered_map<std::type_index, std::function<void(std::ostream &os, std::any const &)>> R;
        R any_visitors;

        streamer_any()
            : any_visitors{
                      to_any_visitor<void>([](std::ostream &os) { os << "{}"; }),
                      to_any_visitor<bool>([](std::ostream &os, bool x) { os << std::boolalpha << x; }),
                      to_any_visitor<int>([](std::ostream &os, int x) { os << x; }),
                      to_any_visitor<int8_t>([](std::ostream &os, int8_t x) { os << x; }),
                      to_any_visitor<int16_t>([](std::ostream &os, int16_t x) { os << x; }),
                      to_any_visitor<int32_t>([](std::ostream &os, int32_t x) { os << x; }),
                      to_any_visitor<int64_t>([](std::ostream &os, int64_t x) { os << x; }),
                      to_any_visitor<unsigned>([](std::ostream &os, unsigned x) { os << x; }),
                      to_any_visitor<uint8_t>([](std::ostream &os, uint8_t x) { os << x; }),
                      to_any_visitor<uint16_t>([](std::ostream &os, uint16_t x) { os << x; }),
                      to_any_visitor<uint32_t>([](std::ostream &os, uint32_t x) { os << x; }),
                      to_any_visitor<uint64_t>([](std::ostream &os, uint64_t x) { os << x; }),
                      to_any_visitor<long>([](std::ostream &os, long x) { os << x; }),
                      to_any_visitor<unsigned long>([](std::ostream &os, unsigned long x) { os << x; }),
                      to_any_visitor<float>([](std::ostream &os, float x) { os << x; }),
                      to_any_visitor<double>([](std::ostream &os, double x) { os << x; }),
                      to_any_visitor<char const *>([](std::ostream &os, char const *s) { os << std::quoted(s); }),
                      to_any_visitor<std::chrono::nanoseconds>([](std::ostream &os, const std::chrono::nanoseconds &x) { hicc::chrono::format_duration(os, x); }),
                      to_any_visitor<std::chrono::microseconds>([](std::ostream &os, const std::chrono::microseconds &x) { hicc::chrono::format_duration(os, x); }),
                      to_any_visitor<std::chrono::milliseconds>([](std::ostream &os, const std::chrono::milliseconds &x) { hicc::chrono::format_duration(os, x); }),
                      to_any_visitor<std::chrono::seconds>([](std::ostream &os, const std::chrono::seconds &x) { hicc::chrono::format_duration(os, x); }),
                      to_any_visitor<std::chrono::minutes>([](std::ostream &os, const std::chrono::minutes &x) { hicc::chrono::format_duration(os, x); }),
                      to_any_visitor<std::chrono::hours>([](std::ostream &os, const std::chrono::hours &x) { hicc::chrono::format_duration(os, x); }),
                      // ... add more handlers for your types ...
              } {}


        template<class T, class F>
        inline std::pair<const std::type_index, std::function<void(std::ostream &os, std::any const &)>>
        to_any_visitor(F const &f) {
            return {
                    std::type_index(typeid(T)),
                    [g = f](std::ostream &os, std::any const &a) {
                        if constexpr (std::is_void_v<T>)
                            g(os);
                        else
                            g(os, std::any_cast<T const &>(a));
                    }};
        }

        inline void process(std::ostream &os, const std::any &a) {
            if (const auto it = any_visitors.find(std::type_index(a.type()));
                it != any_visitors.cend()) {
                it->second(os, a);
            } else {
                std::cout << "Unregistered type " << std::quoted(a.type().name());
            }
        }

        template<class T, class F>
        inline void register_any_visitor(F const &f) {
            std::cout << "Register visitor for type "
                      << std::quoted(typeid(T).name()) << '\n';
            any_visitors.insert(to_any_visitor<T>(f));
        }
    };
#endif


    template<class Visitor>
    inline void visit_any_as(std::any const &, Visitor &&) {
        hicc_throw_line("std::any contained no suitable type, unable to visit");
    }

    template<class First, class... Rest, class Visitor>
    inline void visit_any_as(std::any const &any, Visitor &&visitor) {
        First const *value = std::any_cast<First>(&any);
        if (value) {
            visitor(*value);
        } else {
            visit_any_as<Rest...>(any, std::forward<Visitor>(visitor));
        }
    }

} // namespace hicc::util

namespace hicc::util::factory {

    /**
     * @brief a factory template class
     * @tparam unique 
     * @tparam product_base 
     * @tparam products 
     * @see https://hedzr.com/c++/algorithm/cxx17-factory-pattern/
     */
    template<typename product_base, typename... products>
    class factory final {
    public:
        CLAZZ_NON_COPYABLE(factory);
#if defined(_MSC_VER)
        using string = std::string; // or std::string_view
#else
        using string = std::string_view;
#endif
        template<typename T>
        struct clz_name_t {
            string id = debug::type_name<T>();
            T data;
        };
        using named_products = std::tuple<clz_name_t<products>...>;
        // using _T = typename std::conditional<unique, std::unique_ptr<product_base>, std::shared_ptr<product_base>>::type;

        template<typename... Args>
        static auto create(const string &id, Args &&...args) {
            std::unique_ptr<product_base> result{};

            std::apply([](auto &&...it) {
                ((static_check<decltype(it.data)>()), ...);
            },
                       named_products{});

            std::apply([&](auto &&...it) {
                ((it.id == id ? result = std::make_unique<decltype(it.data)>(args...) : result), ...);
            },
                       named_products{});
            return result;
        }
        template<typename... Args>
        static std::shared_ptr<product_base> make_shared(const string &id, Args &&...args) {
            std::shared_ptr<product_base> ptr = create(id, args...);
            return ptr;
        }
        template<typename... Args>
        static std::unique_ptr<product_base> make_unique(const string &id, Args &&...args) {
            return create(id, args...);
        }
        template<typename... Args>
        static product_base *create_nacked_ptr(const string &id, Args &&...args) {
            return create(id, args...).release();
        }

    private:
        template<typename product>
        static void static_check() {
            static_assert(std::is_base_of<product_base, product>::value, "all products must inherit from product_base");
        }
    }; // class factory

} // namespace hicc::util::factory

#endif //HICC_CXX_HZ_UTIL_HH
