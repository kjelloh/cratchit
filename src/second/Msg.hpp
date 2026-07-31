#pragma once

#include <variant>

namespace tea {

  struct NoMsg {
  }; // NoMsg

  struct UnicodeKeyMsg {
    UnicodeKeyMsg() = delete;
    UnicodeKeyMsg(int cp);
    const char32_t code_point;
  }; // UnicodeKeyMsg

  struct BackspaceKeyMsg {};
  struct EnterKeyMsg {};
  struct EscapeKeyMsg {};
  struct CursorBlinkMsg {};

  using Msg = std::variant<
     NoMsg
    ,UnicodeKeyMsg
    ,BackspaceKeyMsg
    ,EnterKeyMsg
    ,EscapeKeyMsg
    ,CursorBlinkMsg
  >;

} // tea

bool is_no_msg(tea::Msg const& msg);

