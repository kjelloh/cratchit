#pragma once

#include "ViewState.hpp"
#include "AppState.hpp"
#include <immer/vector.hpp>
#include <vector>

namespace tea {

  class Model {
  public:

    Model with_view_state(ViewState const& view_state) const;

    using StateStack = immer::vector<ViewState>;

    // view state stack handling
    Model with_pushed_state(ViewState const& state) const;
    Model with_mutated_state(ViewState const& state) const;
    
    StateStack const& state_stack() const;

    // App state stack handling
    using AppStateStack = immer::vector<AppState>;
    AppStateStack const& app_state_stack() const;
    Model with_mutated_stack(AppStateStack const& app_state_stack) const;
  private:
    StateStack m_state_stack{};
    AppStateStack m_app_state_stack{};
  }; // Model
}
