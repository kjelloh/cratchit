#pragma once

#include "Cmd.hpp"
#include <string>

namespace tea {

    // Cmd (variant) to string conversion for logging and development trace
  std::string cmd_to_string(Cmd const& cmd);

  // Concrete cmd to string conversion for logging and development trace
  // Uses prefix 'concrete' to clarify 'template magic' code that dipatches both on:
  // 1. variant cmd -> concrete cmd
  // 2. concrete_cmd_to_string(concrete cmd) or fallback to concrete cmd type info string if no concrete_cmd_to_string
  std::string concrete_cmd_to_string(TestCmdDescriptor const& c);

} // tea
