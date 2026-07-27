#include "Model.hpp"

namespace tea {

  // public

  Model Model::with_pushed_unicode(char32_t cp) const {
    Model result{*this};
    result.m_code_point_buffer = this->m_code_point_buffer.push_back(cp);
    return result;
  }

  Model Model::with_popped_unicode() const {
    Model result{*this};
    if (this->m_code_point_buffer.size()>0) {
      result.m_code_point_buffer = this->m_code_point_buffer.take(m_code_point_buffer.size()-1);
    }
    return result;
  }

  Model::CodePointBuffer const& Model::code_point_buffer() const {
    return m_code_point_buffer;
  }

  Model Model::with_pushed_state(State const& state) const {
    Model result(*this);
    result.m_state_stack = this->m_state_stack.push_back(state);
    return result;
  }

  Model Model::with_mutated_state(State const& state) const {
    Model result(*this);
    result.m_state_stack = this->m_state_stack.set(
       result.m_state_stack.size()-1
      ,state
    );
    return result;
  }

  Model::StateStack const& Model::state_stack() const {
    return m_state_stack;
  }

  // private:


} // tea
