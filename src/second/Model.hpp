#pragma once

#include "ViewState.hpp"
// #include "AppState.hpp"
#include <immer/vector.hpp>
#include <vector>

namespace tea {

  class Model {
  public:

    using ViewStateStack = immer::vector<ViewState>;
    ViewStateStack const& view_state_stack() const;
    Model with_mutated_view_state_stack(ViewStateStack const& view_state_stack) const;

    Model with_view_state(ViewState const& view_state) const;

    // using AppStateStack = immer::vector<AppState>;
    // AppStateStack const& app_state_stack() const;
    // Model with_mutated_app_state_stack(AppStateStack const& app_state_stack) const;
  private:
    ViewStateStack m_view_state_stack{};
    // AppStateStack m_app_state_stack{};
  }; // Model
}
