#pragma once

namespace first {
  struct CargoBase {
    virtual ~CargoBase() = default;
  };

  struct NoCargo : public CargoBase {
  };

  template <typename T>
  struct CargoImpl : public CargoBase {
    T m_cargo;

    // Constructor for lvalue and rvalue
    // NOTE: See to_cargo regarding brace initialisation not available / possible
    template<typename U>
    CargoImpl(U&& cargo) : m_cargo(std::forward<U>(cargo)) {}

  };

  using Cargo = std::unique_ptr<CargoBase>;

  template <typename T>
  Cargo to_cargo(T&& t) {
      // NOTE 1: make_unique can't use brace initialisation (CargoImpl needs construct from T)
      // NOTE 2: CargoImpl inherits from base class with virtual member = can't brace construct anyways...
      //         This bit me hard!!
      using DecayedT = std::decay_t<T>;
      return std::make_unique<CargoImpl<DecayedT>>(std::forward<T>(t));
  }
} // namespae first