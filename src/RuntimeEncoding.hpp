#pragma once

#include "text/encoding.hpp"
#include <string>
#include <unicode/unistr.h>  // ICU UnicodeString


// TEA Runtime Encoding Configuration
// Centralizes encoding assumptions for the entire application
class RuntimeEncoding {
public:
  using DetectedEncoding = encoding::icu::DetectedEncoding;
  RuntimeEncoding();
  // Get/Set the encoding that TEA::Runtime assumes for terminal I/O
  DetectedEncoding detected_encoding();
  
  // Helper functions
  std::string get_encoding_display_name();
  std::string get_encoding_canonical_name();
    
private:
  DetectedEncoding m_detected_encoding;
};
