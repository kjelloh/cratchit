#pragma once

#include "CargoBase.hpp"
#include "fiscal/amount/HADFramework.hpp"

namespace first {
  using HADsCargo = ConcreteCargo<HeadingAmountDateTransEntries>;
} // namespace first