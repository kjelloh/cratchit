#include "unicode.hpp"

// Leading (high) surrogates: 0xd800 - 0xdbff
// Trailing (low) surrogates: 0xdc00 - 0xdfff
const char32_t LEAD_SURROGATE_MIN  = 0x0000d800;
const char32_t LEAD_SURROGATE_MAX  = 0x0000dbff;
const char32_t TRAIL_SURROGATE_MIN = 0x0000dc00;
const char32_t TRAIL_SURROGATE_MAX = 0x0000dfff;

// Maximum valid value for a Unicode code point
const char32_t CODE_POINT_MAX      = 0x0010ffff;

bool is_surrogate(char32_t cp) {
      return (cp >= LEAD_SURROGATE_MIN && cp <= TRAIL_SURROGATE_MAX);
};

bool is_valid_unicode(char32_t cp) {
  return (cp <= CODE_POINT_MAX && !is_surrogate(cp));
};
