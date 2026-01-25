#pragma once

#include "text/encoding.hpp"
#include <string>
#include <unicode/unistr.h>  // ICU UnicodeString

class RuntimeEncoding {
public:
  using EncodingID = text::encoding::EncodingID;
  RuntimeEncoding(EncodingID const& detected_encoing = EncodingID::Undefined);
  EncodingID detected_encoding();  
  std::string get_encoding_display_name();    
private:
  EncodingID m_detected_encoding;
};

RuntimeEncoding to_inferred_runtime_encoding();
