#pragma once

#include "charset/unicode.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace charset::cp437 {

// One CP437 code point, which is one byte
using CodePoint = std::uint8_t;

// Takes CP437 bytes one at a time and gives back Unicode code points as they
// become available. 0x00-0x1F and 0x7F are the ASCII control characters.
class Decoder {
public:
  // A code point once the bytes pushed so far make one, else nothing.
  // CP437 is one byte per character, so a byte always makes one.
  std::optional<unicode::CodePoint> push(CodePoint byte);

  // Number of failed sequences so far. Every byte is a CP437 character, so
  // decoding CP437 never fails and this is always 0.
  std::size_t failure_count() const { return 0; }
};

// Takes Unicode code points one at a time and gives back their CP437 byte,
// or nothing for a code point CP437 does not have (counted in unmapped_count())
class Encoder {
public:
  std::optional<CodePoint> push(unicode::CodePoint code_point);

  // Number of code points so far that CP437 does not have
  std::size_t unmapped_count() const { return unmapped_count_; }

private:
  std::size_t unmapped_count_ = 0;
};

} // namespace charset::cp437
