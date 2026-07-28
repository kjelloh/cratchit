#include "Model.hpp"
#include "log.hpp"

namespace tea {

  // public

  Model Model::with_pushed_state(ViewState const& state) const {
    Model result(*this);
    result.m_state_stack = this->m_state_stack.push_back(state);
    return result;
  }

  Model Model::with_mutated_state(ViewState const& state) const {
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
