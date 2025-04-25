#pragma once

#include <memory> // std::shared_ptr
#include "msg.hpp" // Msg

namespace first {
  // Cmd: () -> std::optional<Msg>
  // ----------------------------------
  extern std::optional<Msg> Nop();
  // ----------------------------------
  extern std::optional<Msg> DO_QUIT();

} // namespace first