#include "HADsCargo.hpp"
#include "states/StateImpl.hpp"

namespace first {
  namespace cargo {

    // 'visit' for HADsCargo
    template<>
    std::pair<std::optional<State>, Cmd>
    ConcreteCargo<HeadingAmountDateTransEntries>::visit(State const& state) const {
      return state->apply(*this);
    }
  }
}
