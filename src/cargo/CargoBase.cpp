#include "CargoBase.hpp"
#include "states/StateImpl.hpp"

namespace first {
  namespace cargo {

    // // 'visit' for DummyCargo
    // template<>
    // StateUpdateResult ConcreteCargo<DummyPayload>::visit(State const& state) const {
    //   return state->apply(*this);
    // }
  }
}
