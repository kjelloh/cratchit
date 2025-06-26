#include "CargoBase.hpp"
#include "states/StateImpl.hpp"

namespace first {
  namespace cargo {

    // 'visit' for DummyCargo
    template<>
    std::pair<std::optional<State>, Cmd> ConcreteCargo<DummyPayload>::visit(State const& state) const {
      return state->apply(*this);
    }
  }
}
