#include "Model.hpp"
#include "log.hpp"

namespace tea {

  // public

  Model Model::with_pushed_state(State const& state) const {
    Model result(*this);
    result.m_state_stack = this->m_state_stack.push_back(state);
    return result;
  }

  Model Model::with_mutated_state(State const& state) const {
    log_development_trace("Model::with_mutated_state");
    Model result(*this);
    result.m_state_stack = this->m_state_stack.set(
       this->m_state_stack.size()-1
      ,state
    );
    return result;
  }

  Model::StateStack const& Model::state_stack() const {
    return m_state_stack;
  }

  // private:

} // tea
