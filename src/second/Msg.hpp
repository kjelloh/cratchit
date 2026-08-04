/**
 * This is The Elm Architecture (TEA) client provided Msg type
 * The name Msg is used to honor Elm tutorial on the Elm architecture
 * See https://guide.elm-lang.org/architecture/
 */

#pragma once

#include "subscribables.hpp"

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

  // #TEA::events: Concrete event message
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

