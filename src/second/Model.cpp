#include "Model.hpp"

namespace tea {
  Model Model::with_pushed_unicode(char32_t cp) const {
    Model result{*this};
    result.m_code_point_buffer.push_back(cp);
    return result;
  }

  std::vector<char32_t> const& Model::code_point_buffer() const {
    return m_code_point_buffer;
  }


} // tea
