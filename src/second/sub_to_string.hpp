#pragma once

#include "Sub.hpp"

#include <string>

namespace tea {

  // Sub (variant) to string conversion for logging and development trace
  std::string sub_to_string(Sub const& sub);

  // Concrete sub to string conversion for logging and development trace
  // Uses prefix 'concrete' to clarify 'template magic' code that dipatches both on:
  // 1. variant sub -> concrete sub
  // 2. concrete_sub_to_string(concrete sub) or fallback to concrete sub type info string if no concrete_sub_to_string
  std::string concrete_sub_to_string(TestEventDescriptor const& s);

} // tea
