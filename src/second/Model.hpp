#pragma once

#include "State.hpp"
#include <immer/vector.hpp>
#include <vector>

namespace tea {

  class Model {
  public:
    using StateStack = immer::vector<State>;
    using CodePointBuffer = immer::vector<char32_t>;

    // code point buffer handling
    Model with_pushed_unicode(char32_t cp) const;
    Model with_popped_unicode() const;
    CodePointBuffer const& code_point_buffer() const;

    // state stack handling
    Model with_pushed_state(State const& state) const;
    Model with_mutated_state(State const& state) const;
    
    StateStack const& state_stack() const;
  private:

    CodePointBuffer m_code_point_buffer{};

    StateStack m_state_stack{};
  }; // Model
}
