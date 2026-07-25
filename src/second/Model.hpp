#pragma once

#include <immer/vector.hpp>
#include <vector>

namespace tea {
  class Model {
  public:
    using CodePointBuffer = immer::vector<char32_t>;

    Model() = default;

    // mutating factories
    Model with_pushed_unicode(char32_t cp) const;
    Model with_popped_unicode() const;

    CodePointBuffer const& code_point_buffer() const;
  private:

    // Create mutated Model (value + move)
    explicit Model(CodePointBuffer buffer)
      : m_code_point_buffer(std::move(buffer)){}

    CodePointBuffer m_code_point_buffer{};
  }; // Model
}
