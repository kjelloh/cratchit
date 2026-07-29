#include "Model.hpp"
#include "log.hpp"

namespace tea {

  // public

  Model::ViewStateStack const& Model::view_state_stack() const {
    return m_view_state_stack;
  }

  Model Model::with_mutated_view_state_stack(ViewStateStack const& view_state_stack) const {
    Model result(*this);
    result.m_view_state_stack = view_state_stack;
    return result;
  } // with_mutated_view_state_stack


  Model Model::with_view_state(ViewState const& view_state) const {
    Model result(*this);
    result.m_view_state_stack = ViewStateStack{view_state};
    // result.m_app_state_stack = AppStateStack{AppState{}.with_view_state(view_state)};
    return result;
  } // with_view_state

  // Model::AppStateStack const& Model::app_state_stack() const {
  //   return m_app_state_stack;
  // }

  // Model Model::with_mutated_app_state_stack(AppStateStack const& app_state_stack) const {
  //   Model result(*this);
  //   result.m_app_state_stack = app_state_stack;
  //   return result;
  // }

  // private:

} // tea
