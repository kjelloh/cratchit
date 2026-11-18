#include "third/main.hpp" // third::main(...) = the cross-platform cratchit after second (src/third)
#include <string_view>
#include <vector>

// CRATCHIT_WITH_LEGACY (CMake option, ON by default on macOS): compile in the
// variants zeroth, first and second, which build on macOS only. Without them
// cratchit is variant third only.
#if CRATCHIT_WITH_LEGACY

#include "cratchit.h" // first::main(...) = the cratchit variant after zeroth
#include "test/test_runner.hpp"
#include <string>
#include <print>
#include "logger/log.hpp"
#include <spdlog/sinks/rotating_file_sink.h>
#include "zeroth/main.cpp" // zeroth::main(...) = the initial cratchit
#include "second/main.hpp" // second::main(..) = the cratchit variant after first

// The variants zeroth, first and second (and --nop, --version, --test, --gtest...)
static int legacy_main(int argc, char *argv[]) {

    // shoe horned in conan package manager test code (just to keep it for now)
    if (true) {
        conan_gen_print_build_properties();
        std::vector<std::string> vec;
        vec.push_back("test_package");
        cratchit_print_vector(vec);
    }

    int result{0};
    enum class CratchitVariant {
        Undefined
      ,Zeroth
      ,First
      ,Second
      ,Unknown
    }; // CratchitVariant

    CratchitVariant cratchit_variant{CratchitVariant::Undefined};

    // parse arguments for cratchit variant
    for (int i=0;i<argc;++i) {

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

    if (cratchit_variant == CratchitVariant::Second) {
      // The variant 'second' is totally self contained (uses non of the previous code)
      result = second::main(argc, argv);
    }
    else {
      // 'Pre-second' style of init and start (specialized logger:: namespace and all other experimental 'stuff')

      // See https://github.com/gabime/spdlog
      auto rotating_logger = spdlog::rotating_logger_mt("rotating_logger", "logs/rotating_log.txt", 25 * 1024 * 1024, 5);
      spdlog::set_pattern("[%H:%M:%S.%e] %v");

      spdlog::set_default_logger(rotating_logger);

      logger::business("Central main sais << Hello >> --------------------------------- ");

      // Log the arguments
      for (int i=0;i<argc;++i) {
        spdlog::info("Argument {}: '{}'", i, argv[i]);
      }

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

    }

    return result;
}

#endif // CRATCHIT_WITH_LEGACY

int main(int argc, char *argv[]) {
    // --third as the first argument runs variant third. Checked before anything
    // else, so that third's commands (e.g. --sie-to-ar) print nothing but their own.
    bool const third_flag = argc > 1 && std::string_view(argv[1]) == "--third";

#if CRATCHIT_WITH_LEGACY
    if (!third_flag) {
      return legacy_main(argc, argv);
    }
#endif

    // third gets the arguments as a program of its own: --third removed
    std::vector<char*> third_argv(argv, argv + argc);
    if (third_flag) {
      third_argv.erase(third_argv.begin() + 1);
    }
    int const third_argc = static_cast<int>(third_argv.size());
    third_argv.push_back(nullptr); // argv[argc] is a null pointer, as for main
    return third::main(third_argc, third_argv.data());
}
