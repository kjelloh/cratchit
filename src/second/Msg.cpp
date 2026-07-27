#include "Msg.hpp"

namespace tea {

    UnicodeKeyMsg::UnicodeKeyMsg(int cp) : code_point{static_cast<char32_t>(cp)} {}

} // tea

bool is_no_msg(tea::Msg const& msg) {
  return std::visit([](auto const& m) {
      using M = std::remove_cvref_t<decltype(m)>;
      return std::is_same_v<M, tea::NoMsg>;
  }
  ,msg);
}
