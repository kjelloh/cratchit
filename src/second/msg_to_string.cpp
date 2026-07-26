#include "msg_to_string.hpp"

#include <format>

std::string msg_to_string(tea::UnicodeKeyMsg const& m) {
  return std::format("{}:{:X}","UnicodeKeyMsg",static_cast<uint32_t>(m.code_point));
}
