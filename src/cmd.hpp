#pragma once

#include "cross_dependent.hpp" // Cmd,
#include <memory> // std::shared_ptr
#include "msgs/msg.hpp" // Msg

namespace first {

  // Cmd: () -> std::optional<Msg>
  // ----------------------------------
  // Nop-command declaration now in cross_dependent unit
  // extern std::optional<Msg> Nop();
  // ----------------------------------
  extern std::optional<Msg> DO_QUIT();

} // namespace first