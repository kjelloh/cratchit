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
    auto logger = spdlog::rotating_logger_mt("rotating_logger", "logs/rotating_log.txt", 25 * 1024 * 1024, 5);
    spdlog::set_pattern("[%H:%M:%S.%e] %v");

    spdlog::set_default_logger(logger);

    logger::business("Central main sais << Hello >> --------------------------------- ");

    for (int i = 0; i < argc; ++i) {
        spdlog::info("Argument {}: {}", i, argv[i]);
        std::print("Argument {}: {}\n", i, argv[i]);
    }

    // shoe horned in conan package manager test code (just to keep it for now)
    if (true) {
        conan_gen_print_build_properties();
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
    else if (argc > 1 && std::string_view(argv[1]).starts_with("--gtest")) {
        spdlog::info("Running Google Test...");
        std::vector<char*> g_argv;
        return tests::run_all(argc,argv);
    }
    else if (argc > 1 && std::string(argv[1]) == "--test") {
        spdlog::info("Running tests...");
        std::vector<char*> g_argv{};
        g_argv.push_back(argv[0]); // program name
        for (int i=2;i<argc;++i) {
          g_argv.push_back(argv[i]); // args after '--test'
        }
        int g_argc = static_cast<int>(g_argv.size());
        return tests::run_all(g_argc,g_argv.data());
    }
    else while (true) {
        static int loop_count{0};
        logger::business("Central main: ping-pong-count:{}",++loop_count);

        logger::design_insufficiency("Central main test of design_insufficiency log");
        logger::development_trace("Central main test of development_trace log");
        
        // Toggle between zeroth (older) and first (this variant) of cratching
        if (result = zeroth::main(argc, argv);result > 0) break;
        // std::cout << "\nCentral main sais Hello :)" << std::flush;
        if (result = first::main(argc,argv);result == 0) break;
    }

    logger::business("Central main sais >> Bye << -----------------------------------");

    return result;
}
