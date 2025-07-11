#pragma once
#include "tea/event.hpp" // Event
#include "msgs/msg.hpp"  // Msg

namespace first {

  // Subscription: Event -> std::optional<Msg>
  // ----------------------------------
  extern std::optional<Msg> onKey(Event event);

} // namespace first