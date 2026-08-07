#include "Model.hpp"
#include "log.hpp"

namespace app {

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
    return result;
  } // with_view_state

  // private:

} // tea
