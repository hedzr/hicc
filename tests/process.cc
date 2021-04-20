//
// Created by Hedzr Yeh on 2021/4/20.
//

#include "hicc/hz-ios.hh" // allow ostream << istream, ...
#include "hicc/hz-os-io-redirect.hh"
#include "hicc/hz-path.hh"
#include "hicc/hz-process.hh"

#include <fstream>
#include <iostream>

void test_stdout_capture() {
    hicc::process::exec ex("ls -la /tmp/");
    
    std::cout << "stdout:" << '\n';
    std::cout << ex;
    std::cout.flush();
    std::cout.good();
    
    std::cout << '\n';
    std::cout << "done!" << '\n';
    std::cout << '\n';
    std::cout << '\n';
    std::cout.flush();
}
    
void test_stderr_capture() {
    {
        hicc::process::exec ex("ls -la /unknown");

        std::cout << "executed: rec-code = " << ex.ret_code() << '\n';

        std::cout << "stderr:" << '\n';
        // ex.stderr_stream() >> std::noskipws;
        // std::copy(std::istream_iterator<char>(ex.stderr_stream()), std::istream_iterator<char>(), std::ostream_iterator<char>(std::cout));
        std::cout << ex.stderr_stream();

        std::cout << "stdout:" << '\n';
        std::cout << ex;
        std::cout.flush();
    }
    std::cout << '\n';
    std::cout << "done!" << '\n';
    std::cout.flush();
    std::cout << '\n';
    std::cout << '\n';
}

int main() {

    std::fstream file;
    file.open("cout.txt.log", std::ios::out);

    std::cout << "This line written to screen, pwd: " << hicc::path::get_current_dir()
              << "\n";
    {
        hicc::os::pipe_all_to p1(file.rdbuf());
        std::cout << "This line written to file "
                  << "\n";
        std::clog << "hello"
                  << "\n";
    }
    std::cout << "This line written to screen too"
              << "\n";
    std::clog << "world"
              << "\n";
    test_stdout_capture();
    test_stderr_capture();
}