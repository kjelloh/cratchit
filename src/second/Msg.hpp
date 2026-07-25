#pragma once

#include <variant>

namespace tea {

  struct NoMsg {
  }; // NoMsg

  struct UnicodeMsg {
    UnicodeMsg() = delete;
    UnicodeMsg(int cp);
    const char32_t code_point;
  }; // UnicodeMsg

  using Msg = std::variant<
     NoMsg
    ,UnicodeMsg
  >;

} // tea

