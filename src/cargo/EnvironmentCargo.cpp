#include "EnvironmentCargo.hpp"
#include "states/StateImpl.hpp"

namespace first {
  namespace cargo {

    // 'visit' for EnvironmentCargo
    template<>
    std::pair<std::optional<State>, Cmd>
    ConcreteCargo<Environment>::visit(State const& state) const {
      return state->apply(*this);
    }
  }
}
