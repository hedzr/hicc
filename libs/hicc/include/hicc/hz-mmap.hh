//
// Created by Hedzr Yeh on 2021/7/19.
//

#ifndef HICC_CXX_HZ_MMAP_HH
#define HICC_CXX_HZ_MMAP_HH


#if defined(_WIN32)
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include <stdexcept>

#include "hz-path.hh"


namespace hicc::mmap {

    class mmaplib {
    public:
        mmaplib();
        ~mmaplib();
        mmaplib(const char *path);
        mmaplib(mmaplib const &mm) { __copy(mm); }
        mmaplib(mmaplib &&mm) { __copy(mm); }
        mmaplib &operator=(mmaplib const &o) {
            __copy(o);
            return (*this);
        }
        mmaplib &operator=(mmaplib &&o) {
            __copy(o);
            return (*this);
        }

    private:
        void __copy(mmaplib const &mm) {
#if defined(_WIN32)
            hFile_ = ::DuplicateHandle(mm.hFile_);
            hMapping_ = ::DuplicateHandle(mm.hMapping_);
#else
            fd_ = dup2(0, mm.fd_);
#endif
            size_ = mm.size_;
            addr_ = mm.addr_;
        }
        void __copy(mmaplib &&mm) {
#if defined(_WIN32)
            hFile_ = mm.hFile_;
            hMapping_ = mm.hMapping_;
            mm.hFile_ = NULL;
            mm.hMapping_ = NULL;
#else
            fd_ = mm.fd_;
            mm.fd_ = 0;
#endif
            size_ = mm.size_;
            addr_ = mm.addr_;
            mm.addr_ = nullptr;
        }

    public:
        void connect(const char *path);
        void close();

        bool is_open() const;
        std::size_t size() const;
        const char *data() const;

    private:
        void cleanup();

#if defined(_WIN32)
        HANDLE hFile_;
        HANDLE hMapping_;
#else
        int fd_;
#endif
        std::size_t size_;
        void *addr_;
    };

#if defined(_WIN32)
#define MAP_FAILED NULL
#endif


    inline mmaplib::mmaplib()
#if defined(_WIN32)
        : hFile_(NULL)
        , hMapping_(NULL)
#else
        : fd_(-1)
#endif
        , size_(0)
        , addr_(MAP_FAILED) {
    }

    inline mmaplib::mmaplib(const char *path)
#if defined(_WIN32)
        : hFile_(NULL)
        , hMapping_(NULL)
#else
        : fd_(-1)
#endif
        , size_(0)
        , addr_(MAP_FAILED) {
        connect(path);
    }

    inline mmaplib::~mmaplib() { cleanup(); }

    inline void mmaplib::connect(const char *path) {
#if defined(_WIN32)
        hFile_ = ::CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile_ == INVALID_HANDLE_VALUE) {
            std::runtime_error("");
        }

        size_ = ::GetFileSize(hFile_, NULL);

        hMapping_ = ::CreateFileMapping(hFile_, NULL, PAGE_READONLY, 0, 0, NULL);

        if (hMapping_ == NULL) {
            cleanup();
            std::runtime_error("");
        }

        addr_ = ::MapViewOfFile(hMapping_, FILE_MAP_READ, 0, 0, 0);
#else
        fd_ = open(path, O_RDONLY);
        if (fd_ == -1) {
            std::runtime_error("");
        }

        struct stat sb;
        if (fstat(fd_, &sb) == -1) {
            cleanup();
            std::runtime_error("");
        }
        size_ = sb.st_size;

        addr_ = ::mmap(NULL, size_, PROT_READ, MAP_PRIVATE, fd_, 0);
#endif

        if (addr_ == MAP_FAILED) {
            cleanup();
            std::runtime_error("");
        }
    }

    inline bool mmaplib::is_open() const { return fd_ != -1 && addr_ != MAP_FAILED; }

    inline std::size_t mmaplib::size() const { return size_; }

    inline const char *mmaplib::data() const { return (const char *) addr_; }

    inline void mmaplib::close() {
        cleanup();
    }

    inline void mmaplib::cleanup() {
#if defined(_WIN32)
        if (addr_) {
            ::UnmapViewOfFile(addr_);
            addr_ = MAP_FAILED;
        }

        if (hMapping_) {
            ::CloseHandle(hMapping_);
            hMapping_ = NULL;
        }

        if (hFile_ != INVALID_HANDLE_VALUE) {
            ::CloseHandle(hFile_);
            hFile_ = INVALID_HANDLE_VALUE;
        }
#else
        if (addr_ != MAP_FAILED) {
            munmap(addr_, size_);
            addr_ = MAP_FAILED;
        }

        if (fd_ != -1) {
            ::close(fd_);
            fd_ = -1;
        }
#endif
        size_ = 0;
    }


    class mmap {
    public:
        mmap(std::size_t size) {
            _tmpname = path::tmpname_autoincr();
            io::create_sparse_file(_tmpname, size);
            _mm.connect(_tmpname.c_str());
        }
        ~mmap() {
            _mm.close();
            io::delete_file(_tmpname);
        }

        bool is_open() const { return _mm.is_open(); }
        std::size_t size() const { return _mm.size(); }
        const char *data() const { return _mm.data(); }
        std::filesystem::path const &underlying_filename() const { return _tmpname; }

    private:
        mmaplib _mm;
        std::filesystem::path _tmpname;
    }; // class mmap


}; // namespace hicc::mmap


#endif //HICC_CXX_HZ_MMAP_HH
