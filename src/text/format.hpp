#pragma once

#include <string>
#include <optional>

namespace text {
  namespace format {
    std::string to_hex_string(std::size_t value);
    std::ostream& operator<<(std::ostream& os,std::optional<std::string> const& opt_s);
  } // format

}
using text::format::operator<<;