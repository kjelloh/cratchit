#pragma once

#include <vector>

namespace tea {
  class Model {
  public:
    Model with_pushed_unicode(char32_t cp) const;

    std::vector<char32_t> const& code_point_buffer() const;
  private:
    std::vector<char32_t> m_code_point_buffer{};
  }; // Model
}
