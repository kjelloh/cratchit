#include "Model.hpp"
#include "log.hpp"

namespace tea {

  // public

  Model Model::with_view_state(ViewState const& view_state) const {
    Model result(*this);
    result.m_app_state_stack = AppStateStack{AppState{}.with_view_state(view_state)};
    return result;
  } // with_view_state

  Model::AppStateStack const& Model::app_state_stack() const {
    return m_app_state_stack;
  }

  Model Model::with_mutated_stack(AppStateStack const& app_state_stack) const {
    Model result(*this);
    result.m_app_state_stack = app_state_stack;
    return result;
  }

  // private:

} // tea
