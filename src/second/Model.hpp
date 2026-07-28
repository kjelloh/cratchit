#pragma once

#include "ViewState.hpp"
#include "AppState.hpp"
#include <immer/vector.hpp>
#include <vector>

namespace tea {

  class Model {
  public:

    Model with_view_state(ViewState const& view_state) const;

    using AppStateStack = immer::vector<AppState>;
    AppStateStack const& app_state_stack() const;
    Model with_mutated_stack(AppStateStack const& app_state_stack) const;
  private:
    AppStateStack m_app_state_stack{};
  }; // Model
}
