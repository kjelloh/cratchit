#include "cratchit.h" // first::main(...) = the cratchit variant after zeroth
#include "test/test_runner.hpp"
#include <vector>
#include <string>
#include <print>
#include "logger/log.hpp"
#include <spdlog/sinks/rotating_file_sink.h>
#include "zeroth/main.cpp" // zeroth::main(...) = the initial cratchit
#include "second/main.hpp" // second::main(..) = the cratchit variant after first

int main(int argc, char *argv[]) {

    // See https://github.com/gabime/spdlog
    auto logger = spdlog::rotating_logger_mt("rotating_logger", "logs/rotating_log.txt", 25 * 1024 * 1024, 5);
    spdlog::set_pattern("[%H:%M:%S.%e] %v");

    spdlog::set_default_logger(logger);

    logger::business("Central main sais << Hello >> --------------------------------- ");

    // shoe horned in conan package manager test code (just to keep it for now)
    if (true) {
        conan_gen_print_build_properties();
        std::vector<std::string> vec;
        vec.push_back("test_package");
        cratchit_print_vector(vec);
    }

    enum class CratchitVariant {
        Undefined
      ,Zeroth
      ,First
      ,Second
      ,Unknown
    }; // CratchitVariant

    CratchitVariant cratchit_variant{};
    // parse arguments
    for (int i=0;i<argc;++i) {

      spdlog::info("Argument {}: '{}'", i, argv[i]);
      std::print("Argument {}: '{}'\n", i, argv[i]);

      if (std::string(argv[i]) == "--zeroth") {
        cratchit_variant = CratchitVariant::Zeroth;
      }
      else if (std::string(argv[i]) == "--first") {
        cratchit_variant = CratchitVariant::First;
      }
      else if (std::string(argv[i]) == "--second") {
        cratchit_variant = CratchitVariant::Second;
      }
    }

    int result{0};
    if (argc > 1 && std::string(argv[1]) == "--nop") {
        spdlog::info("Cratchit - Compiles and runs");
        std::print("Cratchit - Compiles and runs");
      result = 0;
    }
    else if (argc > 1 && std::string(argv[1]) == "--version") {
        spdlog::info("Cratchit version 0.1.0");
    }
    else if (argc > 1 && std::string_view(argv[1]).starts_with("--gtest")) {
        spdlog::info("Running Google Test...");
        std::vector<char*> g_argv;
        result = !tests::run_all(argc,argv);
    }
    else if (argc > 1 && std::string(argv[1]) == "--test") {
        spdlog::info("Running tests...");
        std::vector<char*> g_argv{};
        g_argv.push_back(argv[0]); // program name
        for (int i=2;i<argc;++i) {
          g_argv.push_back(argv[i]); // args after '--test'
        }
        int g_argc = static_cast<int>(g_argv.size());
        result = !tests::run_all(g_argc,g_argv.data());
    }
    else {

      switch (cratchit_variant) {
        case CratchitVariant::Zeroth: {
          result = result = zeroth::main(argc, argv);
        } break;
        case CratchitVariant::First: {
          result = result = first::main(argc, argv);
        } break;
        case CratchitVariant::Second: {
          result = result = second::main(argc, argv);
        } break;
        default: {
          const auto err_msg = std::format(
             "Failed to know how to start cratchit variant with enum value:{}"
            ,static_cast<int>(cratchit_variant)
          );
          std::cout << "\n" << err_msg;
          logger::design_insufficiency("{}",err_msg);
        } // default
      } // switch

    } // else

    logger::business("Central main sais >> Bye with result:{} << -----------------------------------",result);

    return result;
}
