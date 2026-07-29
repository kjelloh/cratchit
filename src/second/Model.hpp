#pragma once

#include "ViewState.hpp"
#include <immer/vector.hpp>
#include <vector>

namespace tea {

  class Model {
  public:

    using ViewStateStack = immer::vector<ViewState>;
    ViewStateStack const& view_state_stack() const;
    Model with_mutated_view_state_stack(ViewStateStack const& view_state_stack) const;

    Model with_view_state(ViewState const& view_state) const;

  private:
    ViewStateStack m_view_state_stack{};
  }; // Model
}
