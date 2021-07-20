//
// Created by Hedzr Yeh on 2021/7/19.
//

#include "hicc/hz-path.hh"

int main() {
    using namespace hicc::path;
    using namespace hicc::io;

    auto tmpname = tmpname_autoincr("sth_%d");
    auto ofs = create_file(tmpname);
    ofs << "some text" << '\n';
    close_file(ofs);

    std::cout << "file '" << tmpname << "' generated:" << '\n';

    auto ifs = open_file(tmpname);
    std::string data = read_file_content(ifs);
    ifs.close();
    std::cout << data << '\n';

    delete_file(tmpname);
}