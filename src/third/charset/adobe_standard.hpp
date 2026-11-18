#pragma once

#include "charset/unicode.hpp"

#include <cstdint>
#include <optional>

// Adobe StandardEncoding (PDF 32000-1:2008, Annex D), the built-in encoding
// of most Type 1 fonts. Like ASCII, but 0x27 is ’ and 0x60 is ‘.
namespace charset::adobe_standard {

// One byte of StandardEncoding
using CodePoint = std::uint8_t;

// Takes StandardEncoding bytes one at a time and gives back their Unicode code points.
// One byte per character, so a byte always makes one. A byte the encoding
// does not define gives U+FFFD (replacement character).
class Decoder {
public:
  std::optional<unicode::CodePoint> push(CodePoint byte);
};

} // namespace charset::adobe_standard
