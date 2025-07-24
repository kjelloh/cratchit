#pragma once

#include "text/encoding.hpp"
#include <string>


// TEA Runtime Encoding Configuration
// Centralizes encoding assumptions for the entire application
class RuntimeEncoding {
public:
  using DetectedEncoding = encoding::icu::DetectedEncoding;
  // Get/Set the encoding that TEA::Runtime assumes for terminal I/O
  static DetectedEncoding get_assumed_terminal_encoding();
  static void set_assumed_terminal_encoding(DetectedEncoding encoding);
  
  // Helper functions
  static std::string get_encoding_display_name();
  static std::string get_encoding_canonical_name();
  static bool is_utf8_mode();
  static bool supports_unicode();
  
  // Initialize with default assumption (UTF-8)
  static void initialize_defaults();
  
private:
  static DetectedEncoding s_terminal_encoding;
};
