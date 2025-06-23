#pragma once

#include "CargoBase.hpp"
#include "fiscal/amount/HADFramework.hpp"

namespace first {
  namespace cargo {
    struct HADsCargo : public ConcreteCargo<HeadingAmountDateTransEntries> {
      virtual State /* std::pair<std::optional<State>, Cmd> */ visit(State const& state) const override;
    };
  } // namespace cargo
} // namespace first