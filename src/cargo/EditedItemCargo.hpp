#pragma once

#include "CargoBase.hpp"

namespace first {
  namespace cargo {
    enum class ItemMutation {
      UNCHANGED,
      MODIFIED, 
      DELETED
    };
    
    template<typename T>
    struct EditedItem {
      T item;
      ItemMutation mutation;
      
      EditedItem(T item, ItemMutation mutation) 
        : item(item), mutation(mutation) {}
    };
    
    template<typename T>
    using EditedItemCargo = ConcreteCargo<EditedItem<T>>;
  } // namespace cargo
} // namespace first