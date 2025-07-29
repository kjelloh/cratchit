#include "cratchit.h"
#include "test/test_runner.hpp"
#include <vector>
#include <string>
#include <print>
#include "logger/log.hpp"
#include <spdlog/sinks/rotating_file_sink.h>
#include "zeroth/main.cpp" // zeroth variant while refactoring into 'this' variant

int main(int argc, char *argv[]) {

    // See https://github.com/gabime/spdlog
    auto logger = spdlog::rotating_logger_mt("rotating_logger", "logs/rotating_log.txt", 5 * 1024 * 1024, 3);
    spdlog::set_default_logger(logger);

    for (int i = 0; i < argc; ++i) {
        spdlog::info("Argument {}: {}", i, argv[i]);
        std::print("Argument {}: {}\n", i, argv[i]);
    }

    // shoe horned in conan package manager test code (just to keep it for now)
    if (true) {
        cratchit();
        std::vector<std::string> vec;
        vec.push_back("test_package");
        cratchit_print_vector(vec);
    }

    int result{0};
    if (argc > 1 && std::string(argv[1]) == "--nop") {
        spdlog::info("Cratchit - Compiles and runs");
        std::print("Cratchit - Compiles and runs");
      return 0;
    }
    else if (argc > 1 && std::string(argv[1]) == "--version") {
        spdlog::info("Cratchit version 0.1.0");
    }
    else if (argc > 1 && std::string(argv[1]) == "--test") {
        spdlog::info("Running tests...");
        return tests::run_all();
    }
    else while (true) {
        // spdlog::info("\nmain: Before test of logger::cout_proxy");
        logger::cout_proxy << "\nmain: Test of logger::cout_proxy 1";
        logger::cout_proxy << "\nmain: Test of logger::cout_proxy 2";
        logger::cout_proxy << "\nmain: Test of logger::cout_proxy 3" << std::flush;
        spdlog::info("\nmain: After test of logger::cout_proxy");
        // Toggle between zeroth (older) and first (this variant) of cratching
        if (result = zeroth::main(argc, argv);result > 0) break;
        // std::cout << "\nCentral main sais Hello :)" << std::flush;
        if (result = first::main(argc,argv);result == 0) break;
    }
    // std::cout << "\nCentral main sais Bye :)" << std::flush;
    return result;
}
