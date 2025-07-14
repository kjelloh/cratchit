#pragma once

#include "cross_dependent.hpp"
#include <utility> // std::pair

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

    // struct AbstractCargo {
    //   virtual ~AbstractCargo() = default;
    //   virtual StateUpdateResult visit(State const& state) const = 0;
    // };

    // // Concrete Cargo struct for Payload P
    // template <typename P>
    // struct ConcreteCargo : public AbstractCargo {
    //   using payload_type = P;
    //   using pointer_type = ConcreteCargo<P>*;

    //   payload_type m_payload;

    //   // Constructor for lvalue and rvalue
    //   // NOTE: See to_cargo regarding brace initialisation not available (possible)
    //   template <typename U>
    //   explicit ConcreteCargo(U &&payload) : m_payload(std::forward<U>(payload)) {}

    //   // visit declaration.
    //   // See definition(s) = specialisation for each concrete cargo type (e.g., HADsCargo)
    //   virtual StateUpdateResult visit(State const& state) const override;

    // };

    // Dummy payload and cargo (placeholder during development and test)
    // using DummyPayload = std::string;
    // using DummyCargo = ConcreteCargo<DummyPayload>;

    // using Cargo = std::shared_ptr<AbstractCargo>;

    // Cargo visit/apply double dispatch removed (cargo now message passed)
    // template <typename P>
    // Cargo to_cargo(P &&payload) {
    //     // NOTE 1: make_unique can't use brace initialisation ==> ConcreteCargo needs constructor from T
    //     // NOTE 2: ConcreteCargo inherits from base class with virtual member = can't brace construct anyways...
    //     //         This bit me hard!!
    //   using DecayedP = std::decay_t<P>;
    //   return std::make_unique<ConcreteCargo<DecayedP>>(std::forward<P>(payload));
    // }

    // template <typename C>
    // struct to_raw {
    //   using payload_type = typename C::payload_type;
    //   using pointer_type = typename C::pointer_type;
    //   pointer_type operator()(Cargo const& cargo) {
    //     return dynamic_cast<pointer_type>(cargo.get());
    //   }
    // };

  } // namespace cargo

  // using Cargo = cargo::Cargo;

} // namespae first