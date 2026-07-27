#include "msg_to_string.hpp"

#include <format>


std::string msg_to_string(tea::Msg const& msg) {
  return std::visit(
    [](auto const& m) -> std::string {
      const std::type_info& ti = typeid(m);
      return std::format("{}",ti.hash_code());
    }
    ,msg
  );
}

std::string msg_to_string(tea::UnicodeKeyMsg const& m) {
  return std::format("{}:{:X}","UnicodeKeyMsg",static_cast<uint32_t>(m.code_point));
}
