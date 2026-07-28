#pragma once

#include "State.hpp"
#include "AppState.hpp"
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

    using AppStateStack = immer::vector<AppState>;
    AppStateStack const& app_state_stack() const;
    Model with_mutated_stack(AppStateStack const& app_state_stack) const;
  private:
    StateStack m_state_stack{};
    AppStateStack m_app_state_stack{};
  }; // Model
}
