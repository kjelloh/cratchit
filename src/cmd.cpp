#include "cmd.hpp"

namespace first {   
  // Commands
  // ----------------------------------
  std::optional<Msg> Nop() {
    return std::nullopt;
  };

  // ----------------------------------
  std::optional<Msg> DO_QUIT() {
    return QUIT_MSG;
  };
} // namespace first

