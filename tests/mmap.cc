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
        hicc::mmap::detail::mmaplib mm(tmpname.c_str(), false, false);
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

#if 0
#ifdef __GNUC__
namespace std _GLIBCXX_VISIBILITY(default) {
    _GLIBCXX_BEGIN_NAMESPACE_VERSION

    template<typename _CharT, typename _Traits>
    class bf : public std::basic_filebuf<_CharT, _Traits> {
    public:
        int fd() const { return _M_file->fd(); }
    };

    template<typename _CharT, typename _Traits = std::char_traits<_CharT>>
    inline int fd(std::basic_ifstream<_CharT, _Traits> & ifs) {
        auto const *p = ifs.rdbuf();
        return ((bf<_CharT, _Traits> *) p)->fd();
    }

    template<typename _CharT, typename _Traits = std::char_traits<_CharT>>
    inline int fd(std::basic_ofstream<_CharT, _Traits> & ifs) {
        auto const *p = ifs.rdbuf();
        return ((bf<_CharT, _Traits> *) p)->fd();
    }

    _GLIBCXX_END_NAMESPACE_VERSION
} // namespace )
#endif
#endif // 0

void test_3() {
    auto tmpname = hicc::path::tmpname_autoincr();
    hicc::io::create_sparse_file(tmpname, 1024 * 1024 * 1024);

    // auto fs = hicc::io::open_file(tmpname);
    // hicc::mmap::mmap_um<true> mm(std::fd<char>(fs));
    
    auto fd = hicc::mmap::open_file(tmpname.c_str());
    hicc::mmap::mmap_um<true> mm(fd);
    if (mm.is_open()) {

        std::cout << tmpname << ": " << mm.size() << " bytes" << '\n';
        std::cout << std::boolalpha << "is_sparse: " << hicc::path::is_sparse_file(tmpname) << '\n';

        char *ptr = mm.data();
        ptr += 1024 * 1024 * 1024 - 16;
        for (int i = 0; i < 16; i++, ptr++) {
            printf("%02x ", *ptr);
        }

        std::cout << '\n';
    }
}

int main() {
    test_1();
    test_2();
    test_3();
}
