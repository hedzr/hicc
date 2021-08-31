//
// Created by Hedzr Yeh on 2021/8/20.
//

#include "hicc/hz-defs.hh"
#include "hicc/hz-pipeable.hh"

#include "hicc/hz-dbg.hh"
#include "hicc/hz-log.hh"
#include "hicc/hz-x-test.hh"

#include <iostream>
#include <string>

#if OS_WIN
#include <conio.h>
#elif OS_POSIX
#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE 1
#endif
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

/**
 * @brief 
 * @param prompt_string 
 * @return the character user pressed
 * @see https://stackoverflow.com/questions/1449324/how-to-simulate-press-any-key-to-continue/9419951#9419951
 */
inline int press_key(const char *prompt_string = "Press key to continue...") {
    //the struct termios stores all kinds of flags which can manipulate the I/O Interface
    //I have an old one to save the old settings and a new
    static struct termios oldt, newt;
    printf("%s", prompt_string);

    //tcgetattr gets the parameters of the current terminal
    //STDIN_FILENO will tell tcgetattr that it should write the settings
    // of stdin to oldt
    tcgetattr(STDIN_FILENO, &oldt);
    //now the settings will be copied
    newt = oldt;

    //two of the c_lflag will be turned off
    //ECHO which is responsible for displaying the input of the user in the terminal
    //ICANON is the essential one! Normally this takes care that one line at a time will be processed
    //that means it will return if it sees a "\n" or an EOF or an EOL
    newt.c_lflag &= ~(ICANON | ECHO);

    //Those new settings will be set to STDIN
    //TCSANOW tells tcsetattr to change attributes immediately.
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    //now the char wil be requested
    int ch = getchar();

    //the old settings will be written back to STDIN
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    if (ch != '\n') printf("\n");
    return ch;
}
#endif

#define WAIT_FOR_KEY 1

void test_wait_for_key() {
#if defined(PIPE_FROM_STDIN)
    for (std::string line; std::getline(std::cin, line);) {
        std::cout << line << std::endl;
    }
#elif defined(WAIT_FOR_KEY)

#if OS_WIN
    int ch = _getch();
    std::cout << ch << '\n';
#elif OS_POSIX
    // https://www.ibm.com/docs/en/zos/2.1.0?topic=functions-tcgetattr-get-attributes-terminal

    struct termios term;

    if (tcgetattr(STDIN_FILENO, &term) != 0)
        perror("tcgetatt() error");
    else {
        if (term.c_iflag & BRKINT)
            puts("BRKINT is set");
        else
            puts("BRKINT is not set");
        if (term.c_cflag & PARODD)
            puts("Odd parity is used");
        else
            puts("Even parity is used");
        if (term.c_lflag & ECHO)
            puts("ECHO is set");
        else
            puts("ECHO is not set");
        printf("The end-of-file character is x'%02x'\n",
               term.c_cc[VEOF]);
    }

    printf("key got: '%c'\n", press_key());
    printf("END\n");

#else
    hicc_print("unsupported OSes: non-win and non-poxis.");
#endif

#elif defined(WAIT_FOR_KEY_1)
    do {
        std::cout << '\n'
                  << "Press a key to continue...";
    } while (std::cin.get() != '\n');
#else
    hicc_print("unsupported ...");
#endif
}

void test_piped() {
    using namespace hicc::pipeable;

    {
        auto add = piped([](int x, int y) { return x + y; });
        auto mul = piped([](int x, int y) -> int { return x * y; });
        int y1 = 5 | add(2) | mul(5) | add(1);
        hicc_print("    y1 = %d", y1);

        int y2 = 5 | add(2) | piped([](int x, int y) { return x * y; })(5) | piped([](int x) { return x + 1; })();
        // Output: 36
        hicc_print("    y2 = %d", y2);
    }
}


int main() {

    // HICC_TEST_FOR(test_wait_for_key);
    HICC_TEST_FOR(test_piped);

    return 0;
}