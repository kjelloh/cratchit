#include "EnvironmentCargo.hpp"
#include "states/StateImpl.hpp"

namespace first {
  namespace cargo {

    // 'visit' for EnvironmentCargo
    template<>
    StateUpdateResult
    ConcreteCargo<EnvironmentPeriodSlice>::visit(State const& state) const {
      return state->apply(*this);
    }
  }
}
