#include "main.hpp"
#include "log.hpp"

namespace second {
  int main(int argc, char *argv[]) {
    log_development_trace("Hello from second::main");
    log_business("second::cratchit START");
    log_design_insufficiency("This is a test");
    return 0;
  }
} // second