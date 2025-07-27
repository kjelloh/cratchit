#pragma once

#include "text/encoding.hpp"
#include <string>
#include <unicode/unistr.h>  // ICU UnicodeString

class RuntimeEncoding {
public:
  using DetectedEncoding = encoding::icu::DetectedEncoding;
  RuntimeEncoding(DetectedEncoding const& detected_encoing = DetectedEncoding::Undefined);
  DetectedEncoding detected_encoding();  
  std::string get_encoding_display_name();    
private:
  DetectedEncoding m_detected_encoding;
};

RuntimeEncoding to_inferred_runtime_encoding();
