#include "Model.hpp"

namespace tea {
  Model Model::with_pushed_unicode(char32_t cp) const {
    return Model(this->m_code_point_buffer.push_back(cp));
  }

  Model Model::with_popped_unicode() const {

    if (this->m_code_point_buffer.size()==0) return *this;

    return Model(this->m_code_point_buffer.take(m_code_point_buffer.size()-1));
  }

  Model::CodePointBuffer const& Model::code_point_buffer() const {
    return m_code_point_buffer;
  }


} // tea
