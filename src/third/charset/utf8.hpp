#pragma once

#include "charset/unicode.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace charset::utf8 {

// One UTF-8 byte
using CodePoint = std::uint8_t;

// The one to four UTF-8 bytes of one Unicode code point
struct Bytes {
  std::array<CodePoint, 4> bytes{};
  std::size_t size = 0;

  CodePoint const* begin() const { return bytes.data(); }
  CodePoint const* end() const { return bytes.data() + size; }
};

// Takes Unicode code points one at a time and gives back their UTF-8 bytes.
// A surrogate (U+D800-U+DFFF) or a value above U+10FFFF is not a character
// and gives the bytes of U+FFFD (replacement character), counted in
// unmapped_count(). Every character has a UTF-8 form.
class Encoder {
public:
  Bytes push(unicode::CodePoint code_point);

  // Number of code points so far that are not characters
  std::size_t unmapped_count() const { return unmapped_count_; }

private:
  std::size_t unmapped_count_ = 0;
};

// Takes UTF-8 bytes one at a time and gives back Unicode code points as they
// become available. A byte that cannot start or continue a well-formed
// sequence fails it: the sequence so far and that byte are consumed, kept in
// failed_bytes() and give one U+FFFD (replacement character). Overlong forms,
// surrogates and values above U+10FFFF are not well-formed.
class Decoder {
public:
  // A code point once the bytes pushed so far make one, else nothing
  std::optional<unicode::CodePoint> push(CodePoint byte);

  // Ends the input. An unfinished sequence fails and gives U+FFFD.
  std::optional<unicode::CodePoint> finish();

  // Number of failed sequences so far
  std::size_t failure_count() const;

  // The bytes of all failed sequences so far, in the order pushed
  std::vector<CodePoint> const& failed_bytes() const;

private:
  unicode::CodePoint fail(std::optional<CodePoint> breaking_byte);

  Bytes pending_;
  std::size_t expected_size_ = 0;
  unicode::CodePoint code_point_ = 0;
  // Allowed range of the next continuation byte (narrower for some second bytes)
  CodePoint lower_ = 0x80;
  CodePoint upper_ = 0xBF;
  std::size_t failure_count_ = 0;
  std::vector<CodePoint> failed_bytes_;
};

} // namespace charset::utf8
