#pragma once

#include "CargoBase.hpp"
#include "environment.hpp"

namespace first {
  namespace cargo {
    using EnvironmentCargo = ConcreteCargo<EnvironmentPeriodSlice>;    
  } // namespace cargo
} // namespace first