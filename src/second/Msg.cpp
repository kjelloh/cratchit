#include "Msg.hpp"

namespace app {

    UnicodeKeyMsg::UnicodeKeyMsg(int cp) : code_point{static_cast<char32_t>(cp)} {}

} // app

bool is_no_msg(app::Msg const& msg) {
  return std::visit([](auto const& m) {
      using M = std::remove_cvref_t<decltype(m)>;
      return std::is_same_v<M, app::NoMsg>;
  }
  ,msg);
}
