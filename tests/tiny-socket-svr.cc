//
// Created by Hedzr Yeh on 2021/8/31.
//

#include "hicc/hz-defs.hh"

#include "hicc/hz-dbg.hh"
#include "hicc/hz-pool.hh"
#include "hicc/hz-process.hh"

#include <array>
#include <cstring>
#include <iostream>
#include <unordered_map>

#include <fcntl.h>
#if !OS_WIN
#include <netdb.h>
#include <unistd.h>
#if OS_LINUX
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif
#else
// Windows: IOCP
#endif

int port = 2333;


namespace demo {

    typedef int EventType;

#if defined(OS_LINUX) && OS_LINUX
    class Epoll {
    public:
        static const int NO_FLAGS = 0;
        static const int BLOCK_INDEFINITELY = -1;
        static const int MAX_EVENTS = 5;

        Epoll() {
            fileDescriptor = epoll_create1(NO_FLAGS);
            event.data.fd = STDIN_FILENO;
            // 设置epoll event 为EPOLLIN(对应文件描述符可读)， EPOLLPRI(对应文件描述符有紧急事件可读)
            event.events = EPOLLIN | EPOLLPRI;
        }

        int wait() {
            return epoll_wait(fileDescriptor, events.data(), MAX_EVENTS, BLOCK_INDEFINITELY);
        }

        int control() {
            return epoll_ctl(fileDescriptor, EPOLL_CTL_ADD, STDIN_FILENO, &event);
        }

        ~Epoll() {
            close(fileDescriptor);
        }

    private:
        int fileDescriptor;
        struct epoll_event event;
        std::array<epoll_event, MAX_EVENTS> events{};
    };

    class EventHandler {
        // Event Handler

    public:
        int handle_event(EventType et) {
            std::cout << "Event Handler: " << et << std::endl;
            return 0;
        }
    };

    class Reactor {
        // Dispatcher
    public:
        Reactor() {
            epoll.control();
        }

        //注册对应的回调函数到handlers中
        void addHandler(std::string event, EventHandler callback) {
            handlers.emplace(std::move(event), std::move(callback));
        }

        void run() {
            while (true) {
                int numberOfEvents = wait();

                for (int i = 0; i < numberOfEvents; ++i) {
                    std::string input;
                    std::getline(std::cin, input);

                    try {
                        // 根据的具体的事件去找对应的handler，并执行相应的操作
                        handlers.at(input).handle_event(EventType(i));
                    } catch (const std::out_of_range &e) {
                        std::cout << "no  handler for " << input << std::endl;
                    }
                }
            }
        }

    private:
        // handlers Table, 存储事件以及其对应的handlers
        std::unordered_map<std::string, EventHandler> handlers{};
        Epoll epoll;

        int wait() {
            int numberOfEvents = epoll.wait();
            return numberOfEvents;
        }
    };
#endif

} // namespace demo

void test_svr_0() {
#if defined(OS_LINUX) && OS_LINUX
    demo::Reactor reactor;
    reactor.addHandler("a", demo::EventHandler{});
    reactor.addHandler("b", demo::EventHandler{});
    reactor.run();
#endif
}

// based: https://gist.github.com/dtoma/417ce9c8f827c9945a9192900aff805b
namespace v1 {
#if defined(OS_LINUX) && OS_LINUX
    namespace detail {
        class base {
        public:
            base() {}
            virtual ~base() {}

            using on_connected_fn = std::function<void(int infd, struct sockaddr_in &in_addr, int epollfd)>;
            using send_fn = std::function<int(char const *data, int length)>;
            using on_read_fn = std::function<int(int fd, char *data, int length, send_fn sender)>;

            static auto create_and_bind(std::string const &port) {
                struct addrinfo hints;

                memset(&hints, 0, sizeof(struct addrinfo));
                hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
                hints.ai_socktype = SOCK_STREAM; /* TCP */
                hints.ai_flags = AI_PASSIVE;     /* All interfaces */

                struct addrinfo *result;
                int sockt = getaddrinfo(nullptr, port.c_str(), &hints, &result);
                if (sockt != 0) {
                    std::cerr << "[E] getaddrinfo failed\n";
                    return -1;
                }

                struct addrinfo *rp;
                int socketfd;
                for (rp = result; rp != nullptr; rp = rp->ai_next) {
                    socketfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
                    if (socketfd == -1) {
                        continue;
                    }

                    sockt = bind(socketfd, rp->ai_addr, rp->ai_addrlen);
                    if (sockt == 0) {
                        break;
                    }

                    close(socketfd);
                }

                if (rp == nullptr) {
                    std::cerr << "[E] bind failed\n";
                    socketfd = -1;
                }

                freeaddrinfo(result);
                return socketfd;
            }

            static bool make_socket_nonblocking(int socketfd) {
                int flags = fcntl(socketfd, F_GETFL, 0);
                if (flags == -1) {
                    std::cerr << "[ERR] fcntl failed (F_GETFL)\n";
                    return false;
                }

                flags |= O_NONBLOCK;
                int s = fcntl(socketfd, F_SETFL, flags);
                if (s == -1) {
                    std::cerr << "[ERR] fcntl failed (F_SETFL)\n";
                    return false;
                }

                return true;
            }

            template<typename T>
            static inline std::string itoa(T v) {
                std::ostringstream ss;
                ss << v;
                return ss.str();
            }
        };

    } // namespace detail


    class handler {
    public:
        virtual int handle(epoll_event e) = 0;
    };


    template<typename T, int max_events = 32>
    class base_t : public detail::base {
    public:
        base_t() {}
        ~base_t() { close(); }
        using handler_t_nacked = handler;
        using handler_t = std::shared_ptr<handler_t_nacked>;

        T *T_THIS() { return static_cast<T *>(this); }
        explicit operator bool() const { return ok(); }
        // bool operator()() const { return ok(); }
        bool ok() const { return socketfd != -1; }

    protected:
        void add_handler(int fd, handler_t &&h, unsigned int events = EPOLLIN | EPOLLET) {
            _handlers[fd] = h;

            struct epoll_event e;
            e.data.fd = fd;
            e.events = events;
            if (epoll_ctl(this->_epfd, EPOLL_CTL_ADD, fd, &e) < 0) {
                failed("Failed to insert handler to epoll");
            }
        }
        void modify_handler(int fd, unsigned int events) {
            auto it = _handlers.find(fd);
            if (it != _handlers.end()) {
                epoll_event e;
                e.data.fd = fd;
                e.events = events; // todo modify epoll handler
                if (epoll_ctl(this->epollfd, EPOLL_CTL_ADD, fd, &e) < 0) {
                    failed("Failed to insert handler to epoll");
                }
            }
        }
        void remove_handler(int fd) {
            auto it = _handlers.find(fd);
            if (it != _handlers.end()) {
                _handlers.erase(it);
            }
        }

    protected:
        void close() {
            if (socketfd != -1) {
                ::close(socketfd);
                socketfd = -1;
            }
        }
        static int prepare_tcp(int port_number) {
            auto socketfd = create_and_bind(itoa(port_number));
            if (socketfd == -1) {
                return socketfd;
            }

            if (!make_socket_nonblocking(socketfd)) {
                failed("make_socket_nonblocking failed");
                return socketfd;
            }
            if (listen(socketfd, SOMAXCONN) == -1) {
                failed("listen failed");
            }
            return socketfd;
        }
        static int prepare_epoll(int socketfd) {
            auto _epfd = epoll_create1(0);
            if (_epfd == -1) {
                failed("epoll_create1 failed");
                return _epfd;
            }

            struct epoll_event event;
            event.data.fd = socketfd;
            event.events = EPOLLIN | EPOLLET;
            if (epoll_ctl(_epfd, EPOLL_CTL_ADD, socketfd, &event) == -1) {
                failed("epoll_ctl failed");
            }
            return _epfd;
        }
        static void failed(const char *msg) { std::cerr << "[ERR] epoll error: " << msg << '\n'; }
        void failed(int fd, const char *msg = nullptr) {
            epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, nullptr);
            ::close(fd);
            if (msg)
                std::cerr << "[ERR] epoll error: " << msg << '\n';
        }
        bool accept_connection(on_connected_fn const &on_connected) {
            sockaddr_in in_addr;
            socklen_t ca_len = sizeof(in_addr);
            int infd = accept(socketfd, (struct sockaddr *) &in_addr, &ca_len);

            // struct sockaddr in_addr;
            // socklen_t in_len = sizeof(in_addr);
            // int infd = accept(socketfd, &in_addr, &in_len);

            if (infd == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) { // Done processing incoming connections
                    return false;
                }
                std::cerr << "[E] accept failed\n";
                return false;
            }

            // std::string hbuf(NI_MAXHOST, '\0');
            // std::string sbuf(NI_MAXSERV, '\0');
            // if (getnameinfo(&in_addr, in_len,
            //                 const_cast<char *>(hbuf.data()), hbuf.size(),
            //                 const_cast<char *>(sbuf.data()), sbuf.size(),
            //                 NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
            //     std::cout << "[I] Accepted connection on descriptor " << infd << "(host=" << hbuf << ", port=" << sbuf << ")"
            //               << "\n";
            // }

            if (!make_socket_nonblocking(infd)) {
                std::cerr << "[E] make_socket_nonblocking failed\n";
                return false;
            }

            struct epoll_event event;
            event.data.fd = infd;
            event.events = EPOLLIN | EPOLLET;
            if (epoll_ctl(_epfd, EPOLL_CTL_ADD, infd, &event) == -1) {
                std::cerr << "[E] epoll_ctl failed\n";
                return false;
            }

            if (on_connected)
                on_connected(infd, in_addr, _epfd);
            else
                T_THIS()->default_on_connected(infd, in_addr, _epfd);
            return true;
        }
        bool read_data(int fd, on_read_fn const &on_read) {
            char buf[512];
            auto count = read(fd, buf, 512); // or recv() in windows
            if (count == -1) {
                if (errno == EAGAIN) { // read all data
                    return false;
                }
            } else if (count == 0) { // EOF - remote closed connection
                std::cout << "[I] Close " << fd << "\n";
                close();
                return false;
            }

            int sent;
            if (on_read)
                sent = on_read(fd, buf, count, [=](char const *data, int length) {
                    return ::write(fd, data, length);
                });
            else
                sent = T_THIS()->default_on_read(fd, buf, count, [=](char const *data, int length) {
                    return ::write(fd, data, length);
                });
            if (sent > 0) {}
            return true;
        }
        void default_on_connected(int fd, struct sockaddr_in &in_addr, int epollfd) {
            UNUSED(fd, in_addr, epollfd);
            std::cout << "Client connected: " << inet_ntoa(in_addr.sin_addr) << '\n';
        }
        int default_on_read(int fd, char *data, int length, send_fn sender) {
            UNUSED(fd, data, length, sender);

            auto cmd = std::string(data, length);
            std::cout << fd << " says: " << cmd;
            if (cmd.length() >= 5) {
                auto cc = cmd.substr(0, 5);
                if (cc == "\\quit") {
                    return sender("%quit%\n", 7);
                }
                if (cc == "\\time") {
                    std::ostringstream ss;
                    ss << hicc::chrono::now() << '\n';
                    return sender(ss.str().c_str(), ss.str().size());
                }
            }
            return 0;
        }

    protected:
        int socketfd = -1;
        int _epfd = -1;
        std::unordered_map<int, handler_t> _handlers;
        std::array<struct epoll_event, max_events> _events;
    };

    template<typename react_handler, int max_events = 32>
    class epoll_server : public base_t<epoll_server<react_handler, max_events>, max_events> {
    public:
        using super = base_t<epoll_server<react_handler, max_events>>;
        epoll_server(int port_number)
            : _port(port_number) {
        }
        ~epoll_server() { stop(); }

        void stop() { _stopping_cv.kill(); }
        bool start() {
            bool ret = false;

            super::socketfd = super::prepare_tcp(_port);
            if (super::socketfd == -1)
                return ret;

            super::add_handler(super::socketfd, std::make_shared<react_handler>());

            super::_epfd = super::prepare_epoll(super::socketfd);
            if (super::_epfd == -1)
                return ret;

            ret = true;
            using namespace std::literals::chrono_literals;
            auto d = 1ns;
            while (_stopping_cv.wait_for(d)) {
                auto n = epoll_wait(super::_epfd, super::_events.data(), max_events, -1);
                for (int i = 0; i < n; ++i) {
                    auto evt = super::_events[i];
                    auto fd = evt.data.fd;
                    auto h = super::_handlers[fd];
                    h->handle(evt);

                    if (super::_events[i].events & EPOLLERR ||
                        super::_events[i].events & EPOLLHUP ||
                        !(super::_events[i].events & EPOLLIN)) { // error
                        super::failed(fd, "event error");
                    } else if (super::socketfd == fd) { // new connection
                        if (!super::accept_connection(_on_connected)) {
                            super::failed(fd, "accept error");
                        }
                    } else { // data to read
                        if (!super::read_data(fd, _on_read)) {
                            super::failed(fd, "read error");
                        }
                    }
                }
            }
            return ret;
        }

    private:
        int _port;
        hicc::pool::timer_killer _stopping_cv;
        typename super::on_connected_fn _on_connected = nullptr;
        typename super::on_read_fn _on_read = nullptr;
    };


    class echo_handler : public handler {
    public:
        int handle(epoll_event e) {
            UNUSED(e);
            return 0;
        }
    };
#endif
} // namespace v1

void test_epoll_server() {
#if defined(OS_LINUX) && OS_LINUX
    v1::epoll_server<v1::echo_handler> server(port);

    hicc::debug::signal_installer<SIGINT> sii([&](int) {
        server.stop();
    });
    server.start();

    if (server.ok()) {
        //
    }
#endif
}

void test_svr() {
#if !OS_WIN
    hicc::debug::signal_installer<SIGABRT> sia;
#endif
    test_epoll_server();
}

int main() {
    test_svr();
    return 0;
}