#pragma once

#include "charset/unicode.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace charset::utf16 {

// One UTF-16 code unit (two bytes)
using CodeUnit = std::uint16_t;

// One byte of UTF-16 data
using Byte = std::uint8_t;

// Which byte of a code unit comes first
enum class ByteOrder {
  big_endian,    // UTF-16BE, as PDF text strings and the FE FF byte order mark
  little_endian, // UTF-16LE, as the FF FE byte order mark
};

// The two or four bytes of one Unicode code point
struct Bytes {
  std::array<Byte, 4> bytes{};
  std::size_t size = 0;

  Byte const* begin() const { return bytes.data(); }
  Byte const* end() const { return bytes.data() + size; }
};

// Takes Unicode code points one at a time and gives back their UTF-16 bytes.
// A code point above U+FFFF becomes a surrogate pair. A surrogate
// (U+D800-U+DFFF) or a value above U+10FFFF is not a character and gives the
// bytes of U+FFFD (replacement character), counted in unmapped_count().
class Encoder {
public:
  explicit Encoder(ByteOrder order = ByteOrder::big_endian) : order_(order) {}

  Bytes push(unicode::CodePoint code_point);

  // Number of code points so far that are not characters
  std::size_t unmapped_count() const { return unmapped_count_; }

private:
  ByteOrder order_;
  std::size_t unmapped_count_ = 0;
};

// Takes UTF-16 bytes one at a time and gives back Unicode code points as they
// become available. A high surrogate followed by a low surrogate makes one
// code point. A surrogate without its partner fails: the units so far and the
// unit that broke them are consumed and give one U+FFFD (replacement
// character), as in utf8::Decoder. So a lone high surrogate followed
// by "A" gives one U+FFFD, and the "A" is lost.
class Decoder {
public:
  explicit Decoder(ByteOrder order = ByteOrder::big_endian) : order_(order) {}

  // A code point once the bytes pushed so far make one, else nothing
  std::optional<unicode::CodePoint> push(Byte byte);

  // Ends the input. An odd last byte or a lone high surrogate fails and gives U+FFFD.
  std::optional<unicode::CodePoint> finish();

  // Number of failures so far
  std::size_t failure_count() const { return failure_count_; }

private:
  std::optional<unicode::CodePoint> take(CodeUnit unit);
  unicode::CodePoint fail();

  ByteOrder order_;
  std::optional<Byte> first_byte_;      // the first byte of a code unit, waiting for its second
  std::optional<CodeUnit> high_;        // a high surrogate, waiting for its low surrogate
  std::size_t failure_count_ = 0;
};

} // namespace charset::utf16
