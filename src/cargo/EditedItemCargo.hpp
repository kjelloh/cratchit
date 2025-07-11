#pragma once

#include "CargoBase.hpp"

namespace first {
  namespace cargo {    
    template<typename T>
    using EditedItemCargo = ConcreteCargo<EditedItem<T>>;
  } // namespace cargo
} // namespace first