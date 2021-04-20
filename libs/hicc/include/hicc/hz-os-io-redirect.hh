//
// Created by Hedzr Yeh on 2021/4/20.
//

#ifndef HICC_CXX_HZ_OS_IO_REDIRECT_HH
#define HICC_CXX_HZ_OS_IO_REDIRECT_HH

// #include "hz-process.hh"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <istream>

#include <memory>
#include <stdexcept>
#include <streambuf>
#include <string>


namespace hicc::os {

    class pipe_stdin_to {
        std::streambuf *stream_buffer_cout;
        std::streambuf *stream_buffer_cin;

    public:
        pipe_stdin_to(std::streambuf *stream_buffer_file = nullptr) {
            stream_buffer_cout = std::cout.rdbuf();
            stream_buffer_cin = std::cin.rdbuf();
            if (stream_buffer_file)
                std::cin.rdbuf(stream_buffer_file);
        }
        virtual ~pipe_stdin_to() {
            std::cin.rdbuf(stream_buffer_cin);
            std::cout.rdbuf(stream_buffer_cout);
            // std::cout << "This line is written to screen" << std::endl;
        }
    };

    template<std::ostream *stdout = &std::cout>
    class pipe_std_dev_to {
        std::streambuf *stream_buffer_;

    public:
        pipe_std_dev_to(std::streambuf *stream_buffer_file = nullptr) {
            stream_buffer_ = (*stdout).rdbuf();
            if (stream_buffer_file)
                (*stdout).rdbuf(stream_buffer_file);
        }
        virtual ~pipe_std_dev_to() {
            (*stdout).rdbuf(stream_buffer_);
        }
    };


    typedef pipe_std_dev_to<&std::cout> pipe_stdout_to;
    typedef pipe_std_dev_to<&std::cerr> pipe_stderr_to;
    typedef pipe_std_dev_to<&std::clog> pipe_stdlog_to;


    class pipe_all_to : pipe_std_dev_to<&std::cout>
        , pipe_std_dev_to<&std::cerr>
        , pipe_std_dev_to<&std::clog>
        , pipe_stdin_to {
    public:
        pipe_all_to(std::streambuf *output_stream_buffer_file = nullptr, std::streambuf *input_stream_buffer_file = nullptr)
            : pipe_std_dev_to<&std::cout>(output_stream_buffer_file)
            , pipe_std_dev_to<&std::cerr>(output_stream_buffer_file)
            , pipe_std_dev_to<&std::clog>(output_stream_buffer_file)
            , pipe_stdin_to(input_stream_buffer_file) {
        }
        virtual ~pipe_all_to() {}
    };

} // namespace hicc::os

#endif //HICC_CXX_HZ_OS_IO_REDIRECT_HH
