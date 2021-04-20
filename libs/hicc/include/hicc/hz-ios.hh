//
// Created by Hedzr Yeh on 2021/3/1.
//

#ifndef HICC_CXX_HZ_IOS_HH
#define HICC_CXX_HZ_IOS_HH

#include <iomanip>
#include <iosfwd>

#include <fstream>
#include <iostream>
#include <sstream>

template<class _CharT, class _Traits = std::char_traits<_CharT>>
inline std::basic_ostream<_CharT, _Traits> &operator<<(std::basic_ostream<_CharT, _Traits> &os, std::basic_ostringstream<_CharT, _Traits> const &iss) {
    return os << iss.str();
}
template<class _CharT, class _Traits = std::char_traits<_CharT>>
inline std::basic_ostream<_CharT, _Traits> &operator<<(std::basic_ostream<_CharT, _Traits> &os, std::basic_ifstream<_CharT, _Traits> const &ifs) {
    return os << ifs.rdbuf();
}
// template<class _CharT, class _Traits = std::char_traits<_CharT>>
// inline std::basic_ostream<_CharT, _Traits> &operator<<(std::basic_ostream<_CharT, _Traits> &os, std::ifstream const &ifs) {
//     return os << ifs.rdbuf();
// }
template<class _CharT, class _Traits = std::char_traits<_CharT>>
inline std::basic_ostream<_CharT, _Traits> &operator<<(std::basic_ostream<_CharT, _Traits> &os, std::basic_istream<_CharT, _Traits> const &ifs) {

    // ifs >> std::noskipws;
    // std::copy(std::istream_iterator<char>(ifs), std::istream_iterator<char>(), std::ostream_iterator<char>(std::cout));

    if (ifs.rdbuf() != nullptr)
        os << ifs.rdbuf();
    return os;
}


namespace hicc::io {


    class [[maybe_unused]] ios_flags_saver {
    public:
        [[maybe_unused]] explicit ios_flags_saver(std::ostream &os)
            : ios(os)
            , f(os.flags()) {}
        ~ios_flags_saver() { ios.flags(f); }

        ios_flags_saver(const ios_flags_saver &rhs) = delete;
        ios_flags_saver &operator=(const ios_flags_saver &rhs) = delete;

    private:
        std::ostream &ios;
        std::ios::fmtflags f;
    };

    class ios_state_saver {
    public:
        explicit ios_state_saver(std::ostream &os)
            : ios(os) {
            oldState.copyfmt(os);
        }
        ~ios_state_saver() { ios.copyfmt(oldState); }

        ios_state_saver(const ios_state_saver &rhs) = delete;
        ios_state_saver &operator=(const ios_state_saver &rhs) = delete;

    private:
        std::ostream &ios;
        std::ios oldState{nullptr};
    };

} // namespace hicc::io

#endif //HICC_CXX_HZ_IOS_HH
