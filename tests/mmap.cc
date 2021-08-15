//
// Created by Hedzr Yeh on 2021/7/19.
//

#include "hicc/hz-mmap.hh"
#include "hicc/hz-path.hh"
#include "hicc/hz-pool.hh"
#include "hicc/hz-x-test.hh"

void test_1() {
    auto tmpname = hicc::path::tmpname_autoincr();
    hicc::io::create_sparse_file(tmpname, 7 * 1024 * 1024);

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
        hicc::mmap::detail::mmaplib mm(hicc::path::to_filename_h(tmpname).c_str(), false, false);
        std::cout << tmpname << ": " << mm.size() << " bytes" << '\n';

        const auto *ptr = mm.data();
        ptr += 7 * 1024 * 1024 - 16;
        for (int i = 0; i < 16; i++, ptr++) {
            printf("%02x ", *ptr);
        }

        std::cout << '\n';
    }

    { hicc::io::delete_file(tmpname); }
}

void test_2() {
    hicc::mmap::mmap<true> mm(7 * 1024 * 1024);
    if (mm.is_open()) {

        std::cout << mm.underlying_filename() << ": " << mm.size() << " bytes" << '\n';
        std::cout << std::boolalpha << "is_sparse: " << hicc::path::is_sparse_file(mm.underlying_filename()) << '\n';

        char *ptr = mm.data();
        ptr += 7 * 1024 * 1024 - 16;
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
    hicc::io::create_sparse_file(tmpname, 7 * 1024 * 1024);

    // auto fs = hicc::io::open_file(tmpname);
    // hicc::mmap::mmap_um<true> mm(std::fd<char>(fs));

    auto fd = hicc::mmap::open_file(hicc::path::to_filename_h(tmpname).c_str());
    hicc::mmap::mmap_um<true> mm(fd);
    if (mm.is_open()) {

        std::cout << tmpname << ": " << mm.size() << " bytes" << '\n';
        std::cout << std::boolalpha << "is_sparse: " << hicc::path::is_sparse_file(tmpname) << '\n';

        char *ptr = mm.data();
        ptr += 7 * 1024 * 1024 - 16;
        for (int i = 0; i < 16; i++, ptr++) {
            printf("%02x ", *ptr);
        }

        std::cout << '\n';
    }
}

#if !defined(_WIN32)

void errexit(const char *msg) {
    printf("%s\n", msg);
    exit(-1);
}

std::filesystem::path filename = "/tmp/test-setter-1";
const int BUF_SIZE = 100;

void test_watcher() {
    char *mapped;
    int fd;
    struct stat sb;
    if ((fd = open(hicc::path::to_filename_h(filename).c_str(), O_RDONLY)) < 0) {
        errexit("mmap, watcher: open");
    }

    if ((fstat(fd, &sb)) == -1) {
        errexit("mmap, watcher: fstat");
    }

    printf("#%d: file size = %lu\n", fd, (unsigned long)sb.st_size);

    if ((mapped = (char *) mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0)) == (void *) -1) {
        errexit("mmap, watcher");
    }

    using namespace std::literals::chrono_literals;
    std::this_thread::yield();

    int tries = 0;
    while (tries++ < 3) {
        printf("%s\n", mapped);
        std::this_thread::yield();
        std::this_thread::sleep_for(2s); // sleep(2);
    }

    close(fd);
}

void test_setter() {
    char *mapped;

    int fd;
    struct stat sb;
    if ((fd = open(hicc::path::to_filename_h(filename).c_str(), O_RDWR)) < 0) {
        errexit("mmap, setter: open");
    }

    if ((fstat(fd, &sb)) == -1) {
        errexit("mmap, setter: fstat");
    }

    printf("#%d: file size = %lu\n", fd, (unsigned long)sb.st_size);

    if ((mapped = (char *) mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == (void *) -1) {
        errexit("mmap, setter");
    }

    std::this_thread::yield();

    printf("setter: -\n");
    for (int i = 0; i < 40; i++) {
        mapped[i] = '-';
    }

    using namespace std::literals::chrono_literals;
    std::this_thread::yield();
    std::this_thread::sleep_for(2s); // sleep(2);

    printf("setter: 9\n");
    mapped[20] = '9';

    close(fd);
}

void test_setter_and_watch() {
    filename = hicc::path::tmpname_autoincr();
    int fd;
    if ((fd = open(hicc::path::to_filename_h(filename).c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) < 0) {
        printf("create failed (%d): %s\n", errno, hicc::path::to_filename_h(filename).c_str());
        errexit("open+create");
    }

    char buf[BUF_SIZE + 1];
    buf[BUF_SIZE] = 0;
    for (int i = 0; i < BUF_SIZE; i++) {
        buf[i] = '#';
    }

    auto nw = write(fd, buf, BUF_SIZE + 1);
    if (nw < 0) {
        errexit("write");
    }

    // lseek(fd, BUF_SIZE, SEEK_SET);
    // write(fd, &fd, sizeof(fd));

    close(fd);

    printf("begin of controller\n");

    hicc::pool::thread_pool pool(3);
    printf("pool ready\n");
    pool.queue_task([]() { printf("end of empty runner\n"); });
    pool.queue_task(test_watcher);
    pool.queue_task(test_setter);
    // sleep(3);
    std::this_thread::yield();
    pool.join();
}

#else
void test_setter_and_watch() {}
#endif

int main() {
    HICC_TEST_FOR(test_1);
    HICC_TEST_FOR(test_2);
    HICC_TEST_FOR(test_3);
    HICC_TEST_FOR(test_setter_and_watch);
}
