#include "HADsCargo.hpp"
#include "states/StateImpl.hpp"

namespace first {
  namespace cargo {
    std::pair<std::optional<State>, Cmd> HADsCargo::visit(State const& state) const {
      return state->apply(*this);
    }
  }
}
