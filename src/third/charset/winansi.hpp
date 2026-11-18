#pragma once

#include "charset/unicode.hpp"

#include <cstdint>
#include <optional>

// PDF's WinAnsiEncoding (PDF 32000-1:2008, Annex D): Windows-1252, as made
// from Python's cp1252 codec. Bytes 0x00-0x1F and 0x7F are not defined.
namespace charset::winansi {

// One byte of WinAnsiEncoding
using CodePoint = std::uint8_t;

// Takes WinAnsiEncoding bytes one at a time and gives back their Unicode code points.
// One byte per character, so a byte always makes one. A byte the encoding
// does not define gives U+FFFD (replacement character).
class Decoder {
public:
  std::optional<unicode::CodePoint> push(CodePoint byte);
};

} // namespace charset::winansi
