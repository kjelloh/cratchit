#pragma once

#include "CargoBase.hpp"
#include "environment.hpp"

namespace first {
  namespace cargo {
    using EnvironmentCargo = ConcreteCargo<Environment>;    
  } // namespace cargo
} // namespace first