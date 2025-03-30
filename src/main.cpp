#include "cratchit.h"
#include <vector>
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include "zeroth/main.cpp" // zeroth variant while refactoring into 'this' variant

int main(int argc, char *argv[]) {

    // shoe horned in conan package manager test code (just to keep it for now)
    if (true) {
        cratchit();
        std::vector<std::string> vec;
        vec.push_back("test_package");
        cratchit_print_vector(vec);
    }

    // See https://github.com/gabime/spdlog
    auto logger = spdlog::rotating_logger_mt("rotating_logger", "logs/rotating_log.txt", 5 * 1024 * 1024, 3);
    spdlog::set_default_logger(logger);

    int result{};
    while (true) {
        // Toggle between zeroth (older) and first (this variant) of cratching
        if (result = zeroth::main(argc, argv);result > 0) break;
        // std::cout << "\nCentral main sais Hello :)" << std::flush;
        if (result = first::main(argc,argv);result == 0) break;
    }
    // std::cout << "\nCentral main sais Bye :)" << std::flush;
    return result;
}
