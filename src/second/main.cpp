#include "main.hpp"
#include "log.hpp"

namespace second {
  int main(int argc, char *argv[]) {
    log_development_trace("Hello from second::main");
    return 0;
  }
} // second