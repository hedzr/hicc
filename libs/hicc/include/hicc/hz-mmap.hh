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

#if defined(_WIN32)
#define FILE_HANDLE HANDLE
#else
    typedef int FILE_HANDLE;
    const FILE_HANDLE INVALID_HANDLE_VALUE = -1;
#endif

    class mmaplib {
    public:
        mmaplib();
        ~mmaplib();
        mmaplib(const char *path, bool writeable, bool shareable);
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
            mm.hFile_ = INVALID_HANDLE_VALUE;
            mm.hMapping_ = NULL;
#else
            fd_ = mm.fd_;
            mm.fd_ = INVALID_HANDLE_VALUE;
#endif
            size_ = mm.size_;
            addr_ = mm.addr_;
            mm.addr_ = nullptr;
        }

    public:
        void connect(bool writeable, bool shareable, const char *path);
        void connect(bool writeable, bool shareable, FILE_HANDLE fd);
        void close();

        bool is_open() const;
        std::size_t size() const;
        const char *data() const;
        char *data();

    private:
        void cleanup();

#if defined(_WIN32)
        FILE_HANDLE hFile_;
        HANDLE hMapping_;
#else
        FILE_HANDLE fd_;
#endif
        std::size_t size_;
        void *addr_;
    };

#if defined(_WIN32)
#define MAP_FAILED NULL
#endif


    inline mmaplib::mmaplib()
#if defined(_WIN32)
        : hFile_(INVALID_HANDLE_VALUE)
        , hMapping_(NULL)
#else
        : fd_(INVALID_HANDLE_VALUE)
#endif
        , size_(0)
        , addr_(MAP_FAILED) {
    }

    inline mmaplib::mmaplib(const char *path, bool writeable, bool shareable)
#if defined(_WIN32)
        : hFile_(INVALID_HANDLE_VALUE)
        , hMapping_(NULL)
#else
        : fd_(INVALID_HANDLE_VALUE)
#endif
        , size_(0)
        , addr_(MAP_FAILED) {
        connect(writeable, shareable, path);
    }

    inline mmaplib::~mmaplib() { cleanup(); }

    inline void mmaplib::connect(bool writeable, bool shareable, const char *path) {
#if defined(_WIN32)
        hFile_ = ::CreateFileA(path, GENERIC_READ | (writeable ? GENERIC_WRITE : 0),
                               FILE_SHARE_READ | (writeable ? FILE_SHARE_WRITE : 0), NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        connect(writeable, shareable, hFile_);
#else
        fd_ = open(path, writeable ? O_RDWR : O_RDONLY);
        connect(writeable, shareable, fd_);
#endif
    }
    inline void mmaplib::connect(bool writeable, bool shareable, FILE_HANDLE fd) {
#if defined(_WIN32)
        if (fd == INVALID_HANDLE_VALUE) {
            std::runtime_error("");
        }

        size_ = ::GetFileSize(fd, NULL);

        hMapping_ = ::CreateFileMapping(hFile_, NULL, (writeable ? PAGE_READWRITE : PAGE_READONLY), 0, 0, NULL);

        if (hMapping_ == NULL) {
            cleanup();
            std::runtime_error("");
        }

        // https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-mapviewoffile
        addr_ = ::MapViewOfFile(hMapping_, FILE_MAP_READ | (writeable ? FILE_MAP_WRITE : 0), 0, 0, 0);
#else
        if (fd == -1) {
            std::runtime_error("");
        }

        struct stat sb;
        if (fstat(fd, &sb) == -1) {
            cleanup();
            std::runtime_error("");
        }
        size_ = sb.st_size;

        // https://man7.org/linux/man-pages/man2/mmap.2.html
        addr_ = ::mmap(NULL, size_, PROT_READ | (writeable ? PROT_WRITE : 0), shareable ? MAP_SHARED : MAP_PRIVATE, fd, 0);
#endif

        if (addr_ == MAP_FAILED) {
            cleanup();
            std::runtime_error("");
        }
    }

    inline bool mmaplib::is_open() const { return fd_ != -1 && addr_ != MAP_FAILED; }

    inline std::size_t mmaplib::size() const { return size_; }

    inline const char *mmaplib::data() const { return (const char *) addr_; }
    inline char *mmaplib::data() { return (char *) addr_; }

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

        if (fd_ != INVALID_HANDLE_VALUE) {
            ::close(fd_);
            fd_ = INVALID_HANDLE_VALUE;
        }
#endif
        size_ = 0;
    }


    /**
     * @brief wrapper of mmaplib with implicit file management.
     * the corresponding file will be deleted automatically, as 'mmap' deconstructed.
     * 
     * If you looking for a wrapper with connecting to a explicit,
     * external file and without file creating/destroying, use 'mmap_um'. 
     */
    template<bool writeable = false, bool shareable = false>
    class mmap {
    public:
        mmap(std::size_t size) {
            _tmpname = path::tmpname_autoincr();
            io::create_sparse_file(_tmpname, size);
            _mm.connect(writeable, shareable, _tmpname.c_str());
        }
        ~mmap() {
            _mm.close();
            io::delete_file(_tmpname);
        }

        bool is_open() const { return _mm.is_open(); }
        std::size_t size() const { return _mm.size(); }
        std::size_t length() const { return _mm.size(); }
        const char *data() const { return _mm.data(); }
        char *data() { return _mm.data(); }
        std::filesystem::path const &underlying_filename() const { return _tmpname; }

    private:
        mmaplib _mm;
        std::filesystem::path _tmpname;
    }; // class mmap

    /**
     * @brief wrapper of mmaplib without implicit file management
     */
    template<bool writeable = false, bool shareable = false>
    class mmap_um {
    public:
        mmap_um(int fd) {
            _mm.connect(writeable, shareable, fd);
        }
        ~mmap_um() {
            _mm.close();
        }

        bool is_open() const { return _mm.is_open(); }
        std::size_t size() const { return _mm.size(); }
        std::size_t length() const { return _mm.size(); }
        const char *data() const { return _mm.data(); }
        char *data() { return _mm.data(); }

    private:
        mmaplib _mm;
    }; // class mmap_um

} // namespace hicc::mmap


#endif //HICC_CXX_HZ_MMAP_HH
