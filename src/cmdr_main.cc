//
// Created by Hedzr Yeh on 2021/2/14.
//


#include <cmdr11/cmdr11.hh>


#define xVERSION_STRING "0.1.0"


int main(int argc, char *argv[]) {

    auto &cli = cmdr::cli("app2", xVERSION_STRING, "hedzr",
                         "Copyright © 2021 by hedzr, All Rights Reserved.",
                         "A demo app for cmdr-c11 library.",
                         "$ ~ --help");

    try {
        using namespace cmdr::opt;

        cli += sub_cmd{}("server", "s", "svr")
                .description("server operations for listening")
                .group("TCP/UDP/Unix");
        {
            auto &t1 = *cli.last_added_command();

            t1 += opt{(int16_t)(8)}("retry", "r")
                    .description("set the retry times");

            t1 += opt{(uint64_t) 2}("count", "c")
                    .description("set counter value");

            t1 += opt{"localhost"}("host", "H", "hostname", "server-name")
                    .description("hostname or ip address")
                    .group("TCP")
                    .placeholder("HOST[:IP]")
                    .env_vars("HOST");

            t1 += opt{(int16_t) 4567}("port", "p")
                    .description("listening port number")
                    .group("TCP")
                    .placeholder("PORT")
                    .env_vars("PORT", "SERVER_PORT");

            t1 += sub_cmd{}("start", "s", "startup", "run")
                    .description("start the server as a daemon service, or run it at foreground")
                    .on_invoke([](cmdr::opt::cmd const &c, string_array const &remain_args) -> int {
                      UNUSED(c, remain_args);
                      std::cout << c.title() << " invoked.\n";
                      return 0;
                    });

            t1 += sub_cmd{}("stop", "t", "shutdown")
                    .description("stop the daemon service, or stop the server");

            t1 += sub_cmd{}("pause", "p")
                    .description("pause the daemon service");

            t1 += sub_cmd{}("resume", "re")
                    .description("resume the paused daemon service");
            t1 += sub_cmd{}("reload", "r")
                    .description("reload the daemon service");
            t1 += sub_cmd{}("hot-reload", "hr")
                    .description("hot-reload the daemon service without stopping the process");
            t1 += sub_cmd{}("status", "st", "info", "details")
                    .description("display the running status of the daemon service");
        }

    } catch (std::exception &e) {
        std::cerr << "Exception caught for duplicated cmds/args: " << e.what() << '\n';
        CMDR_DUMP_STACK_TRACE(e);
    }

    return cli.run(argc, argv);
}
