#include "main.hpp"
#include "log.hpp"
#include "custom_raylib_log_callback.hpp"
#include "playground_raylib_main.hpp"
#include "cratchit_raylib_main.hpp"

#include <iostream> // std::cout,...
#include <iomanip> // std::quoted,...

namespace second {

  int main(int argc, char *argv[]) {
    int posix_result{};

    try {
      log_business("second::cratchit START");
      log_development_trace("Hello from second::main");
      log_design_insufficiency("This is a test");

      log_business("Test with formatting int value {}",2);
      log_development_trace("Test with formatting int value {}",3);
      log_design_insufficiency("Test with formatting int value {}",4);

      // Hard code a selection of (switch between) production or playground (experiment and investigate) main
      if (true) {
        tea::CratchitRaylibApp app{};
        posix_result = app.run(argc,argv);
      }
      else {
        PlaygroundRaylibApp app{};
        posix_result = app.run(argc,argv);
      }

    }
    catch (const std::exception& e) {
      log_business("second::main: Exception:{}",e.what());
      std::cout << "\nsecond::main(): FAILED: Excpetion " << std::quoted(e.what());
    }
    catch (...) {
      log_business("second::main: General Excpetion:...");
      std::cout << "\nsecond::main(): FAILED: General Excpetion";
    }
    log_flush();

    return posix_result; 
  }
} // second