#pragma once

#include "State.hpp"
#include <immer/vector.hpp>
#include <vector>

namespace tea {

  class Model {
  public:
    using StateStack = immer::vector<State>;

    // state stack handling
    Model with_pushed_state(State const& state) const;
    Model with_mutated_state(State const& state) const;
    
    StateStack const& state_stack() const;
  private:
    StateStack m_state_stack{};
  }; // Model
}
