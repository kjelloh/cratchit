#include "HADsCargo.hpp"
#include "states/StateImpl.hpp"

namespace first {
  namespace cargo {
    Cmd HADsCargo::visit(State const& state) const {
      state->apply(*this);
      return {};
    }
  }
}
