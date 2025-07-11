#include "EditedItemCargo.hpp"
#include "states/StateImpl.hpp"
#include "fiscal/amount/HADFramework.hpp"

namespace first {
  namespace cargo {

    // 'visit' specialization for EditedItemCargo<HAD>
    template<>
    StateUpdateResult
    ConcreteCargo<EditedItem<HeadingAmountDateTransEntry>>::visit(State const& state) const {
      return state->apply(*this);
    }

  } // namespace cargo
} // namespace first