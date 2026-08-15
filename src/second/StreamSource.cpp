#include "StreamSource.hpp"

#include "DesignInsufficiencyException.hpp"

StreamSource::StreamSource(Url url,OwningIStreamPtr in_ptr)
  : m_url{url}, m_in_ptr{std::move(in_ptr)} {}

OwningIStreamPtr::element_type& StreamSource::stream() {
  if (!m_in_ptr) throw DesignInsufficiencyException("StreamSource::stream: Expected existing in_ptr");
  if (!(*m_in_ptr)) throw DesignInsufficiencyException("StreamSource::stream: Expected heatlhy in-stream");
  return *m_in_ptr;
}
Url const& StreamSource::url() const {
  return m_url;
}

