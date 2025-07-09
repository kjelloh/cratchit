#include "EditedItemCargo.hpp"
#include "states/StateImpl.hpp"
#include "fiscal/amount/HADFramework.hpp"

namespace first {
  namespace cargo {

    // 'visit' specialization for EditedItemCargo<HAD>
    template<>
    std::pair<std::optional<State>, Cmd>
    ConcreteCargo<EditedItem<HeadingAmountDateTransEntry>>::visit(State const& state) const {
      return state->apply(*this);
    }

  } // namespace cargo
} // namespace first