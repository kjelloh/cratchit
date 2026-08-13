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


  Model Model::with_pushed_view_state(ViewState const& view_state) const {
    return this->with_mutated_view_state_stack(
      this->m_view_state_stack.push_back(view_state)
    );
  } // with_view_state

  Model Model::with_top_view_state(ViewState const& view_state) const {
    return this->with_mutated_view_state_stack(
      this->m_view_state_stack.set(
         this->m_view_state_stack.size()-1
        ,view_state
      )
    );
  }


  // private:

} // tea
