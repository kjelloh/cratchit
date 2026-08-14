/**
 * This is The Elm Architecture (TEA) client provided Model type
 * The name Model is used to honor Elm tutorial on the Elm architecture
 * See https://guide.elm-lang.org/architecture/
 */

#pragma once

#include "ViewState.hpp"
#include <immer/vector.hpp>
#include <vector>

namespace app {

  class Model {
  public:

    using ViewStateStack = immer::vector<ViewState>;
    ViewStateStack const& view_state_stack() const;

    Model with_mutated_view_state_stack(ViewStateStack const& view_state_stack) const;
    Model with_pushed_view_state(ViewState const& view_state) const;

    Model with_top_view_state(ViewState const& view_state) const;

  private:
    ViewStateStack m_view_state_stack{};
  }; // Model

} // app
