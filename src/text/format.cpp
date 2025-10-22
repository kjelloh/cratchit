#include "format.hpp"
#include <format>

namespace text {
  namespace format {
    std::string to_hex_string(std::size_t value) {
      return std::format("{:x}",value);
    }
  }
}