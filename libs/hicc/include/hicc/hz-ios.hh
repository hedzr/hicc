//
// Created by Hedzr Yeh on 2021/3/1.
//

#ifndef HICC_CXX_HZ_IOS_HH
#define HICC_CXX_HZ_IOS_HH

#include <iomanip>
#include <iosfwd>

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
