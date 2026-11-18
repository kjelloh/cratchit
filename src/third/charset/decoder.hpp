#pragma once

#include "charset/cp437.hpp"
#include "charset/encoding.hpp"
#include "charset/unicode.hpp"
#include "charset/utf16.hpp"
#include "charset/utf8.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>

namespace charset {

// One of the decoders, as a value
using AnyDecoder = std::variant<utf8::Decoder, utf16::Decoder, cp437::Decoder>;

// The decoder of an encoding
AnyDecoder make_any_decoder(Encoding encoding);

// A decoder chosen at runtime. Takes bytes one at a time and gives back
// Unicode code points as they become available, as the decoder it holds.
class Decoder {
public:
  explicit Decoder(Encoding encoding) : decoder_(make_any_decoder(encoding)) {}

  // A code point once the bytes pushed so far make one, else nothing
  std::optional<unicode::CodePoint> push(std::uint8_t byte);

  // Ends the input. Gives U+FFFD if an unfinished sequence was left.
  std::optional<unicode::CodePoint> finish();

  // Number of failed input sequences so far
  std::size_t failure_count() const;

private:
  AnyDecoder decoder_;
};

} // namespace charset
