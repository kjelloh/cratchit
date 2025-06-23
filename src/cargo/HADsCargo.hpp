#pragma once

#include "CargoBase.hpp"
#include "fiscal/amount/HADFramework.hpp"

namespace first {
  namespace cargo {
    using HADsCargo = ConcreteCargo<HeadingAmountDateTransEntries>;
  } // namespace cargo
} // namespace first