//
// Created by Hedzr Yeh on 2021/7/19.
//

#include "hicc/hz-mmap.hh"
#include "hicc/hz-path.hh"

void test_1() {
    auto tmpname = hicc::path::tmpname_autoincr();
    hicc::io::create_sparse_file(tmpname, 1024 * 1024 * 1024);

#if 0
    {
        auto fs = hicc::io::open_file_for_write(tmpname, std::ios_base::binary | std::ios_base::out);
        fs.seekp(1024 * 1024 * 1024 - 1);
        char ch = 'c';
        fs.write(&ch, 1);
        fs.close();
    }
#endif

    {
        hicc::mmap::mmaplib mm(tmpname.c_str(), false, false);
        std::cout << tmpname << ": " << mm.size() << " bytes" << '\n';

        const auto *ptr = mm.data();
        ptr += 1024 * 1024 * 1024 - 16;
        for (int i = 0; i < 16; i++, ptr++) {
            printf("%02x ", *ptr);
        }

        std::cout << '\n';
    }

    { hicc::io::delete_file(tmpname); }
}

void test_2() {
    hicc::mmap::mmap<true> mm(1024 * 1024 * 1024);
    if (mm.is_open()) {

        std::cout << mm.underlying_filename() << ": " << mm.size() << " bytes" << '\n';
        std::cout << std::boolalpha << "is_sparse: " << hicc::path::is_sparse_file(mm.underlying_filename()) << '\n';

        char *ptr = mm.data();
        ptr += 1024 * 1024 * 1024 - 16;
        for (int i = 0; i < 16; i++, ptr++) {
            *ptr = (char) i;
            printf("%02x ", *ptr);
        }

        std::cout << '\n';
    }
}

int main() {
    test_1();
    test_2();
}
