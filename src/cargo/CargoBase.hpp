#pragma once

namespace first {

  struct AbstractCargo {
    virtual ~AbstractCargo() = default;
  };

  struct NoCargo : public AbstractCargo {
  };

  template <typename T>
  struct ConcreteCargo : public AbstractCargo {
    T m_payload;

    // Constructor for lvalue and rvalue
    // NOTE: See to_cargo regarding brace initialisation not available (possible)
    template <typename U>
    explicit ConcreteCargo(U &&payload) : m_payload(std::forward<U>(payload)) {}
  };

  using Cargo = std::unique_ptr<AbstractCargo>;

  template <typename T>
  Cargo to_cargo(T &&payload) {
      // NOTE 1: make_unique can't use brace initialisation (ConcreteCargo needs constructor from T)
      // NOTE 2: ConcreteCargo inherits from base class with virtual member = can't brace construct anyways...
      //         This bit me hard!!
    using DecayedT = std::decay_t<T>;
    return std::make_unique<ConcreteCargo<DecayedT>>(std::forward<T>(payload));
  }

} // namespae first