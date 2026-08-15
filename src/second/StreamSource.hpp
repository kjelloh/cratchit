#pragma once

#include "OwningIStreamPtr.hpp"
#include "Url.hpp"

class StreamSource {
public:
  StreamSource() = delete;
  StreamSource(StreamSource const&) = delete;
  StreamSource(StreamSource&&) = default;
  StreamSource(Url url,OwningIStreamPtr in_ptr);

  OwningIStreamPtr::element_type& stream();
  Url const& url() const;

private:
  Url m_url;
  OwningIStreamPtr m_in_ptr;
}; // StreamSource