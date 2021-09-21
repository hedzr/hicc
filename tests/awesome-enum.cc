//
// Created by Hedzr Yeh on 2021/8/31.
//

#include "hicc/hz-common.hh"

#include "hicc/hz-dbg.hh"
#include "hicc/hz-log.hh"
#include "hicc/hz-x-test.hh"

enum class Animal { DOG,
                    CAT = 100,
                    HORSE = 1000,
                    __COUNT };
inline std::ostream &operator<<(std::ostream &os, Animal value) {
    std::string enumName = "Animal";
    std::string str = "DOG, CAT= 100, HORSE = 1000";
    int len = (int) str.length(), val = -1;
    std::map<int, std::string> maps;
    std::ostringstream temp;
    for (int i = 0; i < len; i++) {
        if (isspace(str[i])) continue;
        if (str[i] == ',') {
            std::string s0 = temp.str();
            auto ix = s0.find('=');
            if (ix != std::string::npos) {
                auto s2 = s0.substr(ix + 1);
                s0 = s0.substr(0, ix);
                std::stringstream ss(s2);
                ss >> val;
            } else
                val++;
            maps.emplace(val, s0);
            temp.str(std::string());
        } else
            temp << str[i];
    }
    std::string s0 = temp.str();
    auto ix = s0.find('=');
    if (ix != std::string::npos) {
        auto s2 = s0.substr(ix + 1);
        s0 = s0.substr(0, ix);
        std::stringstream ss(s2);
        ss >> val;
    } else
        val++;
    maps.emplace(val, s0);
    os << enumName << "::" << maps[(int) value];
    return os;
}

AWESOME_MAKE_ENUM(Week,
                  Sunday, Monday,
                  Tuesday, Wednesday, Thursday, Friday, Saturday)

void test_awesome_enum() {
    auto dog = Animal::DOG;
    std::cout << dog << '\n';
    std::cout << Animal::HORSE << '\n';
    std::cout << Animal::CAT << '\n';

    std::cout << Week::Saturday << '\n';
}

int main() {

    HICC_TEST_FOR(test_awesome_enum);

    return 0;
}
