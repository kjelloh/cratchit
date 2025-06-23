#pragma once

#include "CargoBase.hpp"
#include "fiscal/amount/HADFramework.hpp"

namespace first {
  namespace cargo {
    struct HADsCargo : public ConcreteCargo<HeadingAmountDateTransEntries> {
      virtual void visit(State const& state) const override;
    };
  } // namespace cargo
} // namespace first