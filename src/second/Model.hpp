#pragma once

#include <vector>

namespace tea {
  class Model {
  public:
    // mutating factories
    Model with_pushed_unicode(char32_t cp) const;
    Model with_popped_unicode() const;

    std::vector<char32_t> const& code_point_buffer() const;
  private:
    std::vector<char32_t> m_code_point_buffer{};
  }; // Model
}
