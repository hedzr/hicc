//
// Created by Hedzr Yeh on 2021/3/1.
//

#ifndef HICC_CXX_HZ_LOG_HH
#define HICC_CXX_HZ_LOG_HH

#include "hz-defs.hh"

#include "hz-terminal.hh"
#include "hz-util.hh"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <tuple>


namespace hicc::log {

    namespace detail {
        class Log final : public util::singleton<Log> {
        public:
            explicit Log(typename util::singleton<Log>::token) {}
            ~Log() = default;

            // [[maybe_unused]] hicc::terminal::colors::colorize _c;

            template<class... Args>
            void log([[maybe_unused]] const char *fmt, [[maybe_unused]] Args const &...args) {
                // std::fprintf(std::stderr)
            }
            void vdebug(const char *file, int line, const char *func,
                        char const *fmt, va_list args) {
                // auto t = cross::time(nullptr);
                char time_buf[100];
#if _MSC_VER
                struct tm tm_ {};
                auto _tm = &tm_;
                time_t vt = time(nullptr);
                gmtime_s(_tm, &vt);
#else
                auto *_tm = cross::gmtime();
#endif
                std::strftime(time_buf, sizeof time_buf, "%D %T", _tm);

                va_list args2;
                va_copy(args2, args);
                std::vector<char> buf((std::size_t) std::vsnprintf(nullptr, 0, fmt, args) + 1);
                std::vsnprintf(buf.data(), buf.size(), fmt, args2);
                va_end(args2);

                using namespace hicc::terminal;
                std::printf("%s"
                            "%s %s:"
                            "%s"
                            " %s  %s:%d "
                            "%s"
                            "(%s)"
                            "%s"
                            "\n",
                            fg_light_gray,
                            time_buf,
                            // _c.dim().s("[debug]").as_string().c_str(),
                            "[debug]",
                            fg_reset_all,
                            buf.data(),
                            file, line,
                            fg_light_gray, func, fg_reset_all);
            }
        };
    } // namespace detail

#if 0
    class log {
    public:
        template<class... Args>
        static void print(const char *fmt, Args const &...args) {
            xlog().template log(fmt, args...);
        }

        // static void vdebug(char const *fmt, va_list args) {
        //     xlog().vdebug(fmt, args);
        // }
        static void debug(char const *fmt, ...) {
            // auto t = cross::time(nullptr);
            char time_buf[100];
            // std::strftime(time_buf, sizeof time_buf, "%D %T", cross::gmtime());
#if _MSC_VER
            struct tm tm_ {};
            auto _tm = &tm_;
            time_t vt = time(nullptr);
            gmtime_s(_tm, &vt);
#else
            auto _tm = cross::gmtime();
#endif
            std::strftime(time_buf, sizeof time_buf, "%D %T", _tm);

            va_list args1;
            va_start(args1, fmt);
            va_list args2;
            va_copy(args2, args1);
            std::vector<char> buf((std::size_t) std::vsnprintf(nullptr, 0, fmt, args1) + 1);
            va_end(args1);
            std::vsnprintf(buf.data(), buf.size(), fmt, args2);
            va_end(args2);

            std::printf("%s [debug]: %s\n", time_buf, buf.data());
        }

    private:
        static detail::Log &xlog() { return detail::Log::instance(); }
    }; // class log
#endif

    class holder {
        const char *_file;
        int _line;
        const char *_func;

    public:
        holder(const char *file, int line, const char *func)
            : _file(file)
            , _line(line)
            , _func(func) {}

        void operator()(char const *fmt, ...) {
            va_list va;
            va_start(va, fmt);
            xlog().vdebug(_file, _line, _func, fmt, va);
            va_end(va);
        }

    private:
        static detail::Log &xlog() { return detail::Log::instance(); }
    };
    // Logger log;
} // namespace hicc::log

#if defined(_DEBUG)
#if defined(_MSC_VER)
#define hicc_debug(...) hicc::log::holder(__FILE__, __LINE__, __FUNCSIG__)(__VA_ARGS__)
#else
#define hicc_debug(...) hicc::log::holder(__FILE__, __LINE__, __PRETTY_FUNCTION__)(__VA_ARGS__)
#endif
#else
#if defined(__GNUG__) || defined(_MSC_VER)
#define hicc_debug(...) \
    (void) 0
#else
#define hicc_debug(...)                                                                       \
    _Pragma("GCC diagnostic push")                                                            \
            _Pragma("GCC diagnostic ignored \"-Wunused-value\"") do { (void) (__VA_ARGS__); } \
    while (0)                                                                                 \
    _Pragma("GCC diagnostic pop")
#endif
#endif

#if defined(HICC_ENABLE_VERBOSE_LOG)
// inline void debug(char const *fmt, ...) {
//     va_list va;
//     va_start(va, fmt);
//     hicc::log::log::vdebug(fmt, va);
//     va_end(va);
// }
#if defined(_MSC_VER)
#define hicc_verbose_debug(...) hicc::log::holder(__FILE__, __LINE__, __FUNCSIG__)(__VA_ARGS__)
#else
#define hicc_verbose_debug(...) hicc::log::holder(__FILE__, __LINE__, __PRETTY_FUNCTION__)(__VA_ARGS__)
#endif
#else
// #define hicc_verbose_debug(...)
//     _Pragma("GCC diagnostic push")
//             _Pragma("GCC diagnostic ignored \"-Wunused-value\"") do { (void) (__VA_ARGS__); }
//     while (0)
//     _Pragma("GCC diagnostic pop")

//#define hicc_verbose_debug(...) (void)(__VA_ARGS__)

template<typename... Args>
inline void hicc_verbose_debug([[maybe_unused]] Args &&...args) { (void) (sizeof...(args)); }
#endif

#endif //HICC_CXX_HZ_LOG_HH
