#include "format.hpp"
#include <format>
#include <iomanip> // std::quoted,...

namespace text {
  namespace format {
    std::string to_hex_string(std::size_t value) {
      return std::format("{:x}",value);
    }

    // Helper for stream output of optional string
    std::ostream& operator<<(std::ostream& os,std::optional<std::string> const& opt_s) {
      if (opt_s) {
        os << std::quoted(*opt_s);
      }
      return os;
    }

  }
}