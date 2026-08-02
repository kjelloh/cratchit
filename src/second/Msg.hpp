#pragma once

#include "subscribeables.hpp"

#include <variant>

namespace tea {

  struct NoMsg {};
  struct TickMsg {};

  struct UnicodeKeyMsg {
    UnicodeKeyMsg() = delete;
    UnicodeKeyMsg(int cp);
    const char32_t code_point;
  }; // UnicodeKeyMsg

  struct BackspaceKeyMsg {};
  struct EnterKeyMsg {};
  struct EscapeKeyMsg {};
  struct CursorBlinkMsg {};
  struct TestEventMsg {
    TestEventDescriptor::payload_type payload;
  }; // TestEventMsg

  using Msg = std::variant<
     NoMsg
    ,TickMsg
    ,UnicodeKeyMsg
    ,BackspaceKeyMsg
    ,EnterKeyMsg
    ,EscapeKeyMsg
    ,CursorBlinkMsg
    ,TestEventMsg
  >;

} // tea

bool is_no_msg(tea::Msg const& msg);

