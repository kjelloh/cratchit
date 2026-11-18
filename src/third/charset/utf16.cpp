#include "charset/utf16.hpp"

namespace charset::utf16 {

namespace {

bool is_high_surrogate(unicode::CodePoint value) { return value >= 0xD800 && value <= 0xDBFF; }
bool is_low_surrogate(unicode::CodePoint value) { return value >= 0xDC00 && value <= 0xDFFF; }

void append(Bytes& bytes, CodeUnit unit, ByteOrder order) {
  auto const high = static_cast<Byte>(unit >> 8);
  auto const low = static_cast<Byte>(unit & 0xFF);
  bytes.bytes[bytes.size++] = order == ByteOrder::big_endian ? high : low;
  bytes.bytes[bytes.size++] = order == ByteOrder::big_endian ? low : high;
}

} // namespace

Bytes Encoder::push(unicode::CodePoint code_point) {
  Bytes result;
  if ((code_point >= 0xD800 && code_point <= 0xDFFF) || code_point > 0x10FFFF) {
    ++unmapped_count_;
    code_point = unicode::replacement_character;
  }
  if (code_point < 0x10000) {
    append(result, static_cast<CodeUnit>(code_point), order_);
    return result;
  }
  auto const value = code_point - 0x10000;
  append(result, static_cast<CodeUnit>(0xD800 + (value >> 10)), order_);
  append(result, static_cast<CodeUnit>(0xDC00 + (value & 0x3FF)), order_);
  return result;
}

std::optional<unicode::CodePoint> Decoder::push(Byte byte) {
  if (!first_byte_) {
    first_byte_ = byte;
    return std::nullopt;
  }
  auto const first = static_cast<CodeUnit>(*first_byte_);
  first_byte_.reset();
  auto const unit = order_ == ByteOrder::big_endian ? static_cast<CodeUnit>((first << 8) | byte)
                                                    : static_cast<CodeUnit>((byte << 8) | first);
  return take(unit);
}

std::optional<unicode::CodePoint> Decoder::take(CodeUnit unit) {
  if (high_) {
    if (!is_low_surrogate(unit)) {
      return fail(); // The high surrogate is alone; this unit goes with it
    }
    auto const code_point = 0x10000 + ((static_cast<unicode::CodePoint>(*high_) - 0xD800) << 10) +
                            (static_cast<unicode::CodePoint>(unit) - 0xDC00);
    high_.reset();
    return code_point;
  }
  if (is_high_surrogate(unit)) {
    high_ = unit;
    return std::nullopt;
  }
  if (is_low_surrogate(unit)) {
    return fail();
  }
  return unicode::CodePoint{unit};
}

// Consumes the waiting high surrogate, if any, with the unit that broke it
unicode::CodePoint Decoder::fail() {
  high_.reset();
  ++failure_count_;
  return unicode::replacement_character;
}

std::optional<unicode::CodePoint> Decoder::finish() {
  // An odd last byte and a waiting high surrogate are one failure together
  auto const failed = first_byte_.has_value() || high_.has_value();
  first_byte_.reset();
  if (!failed) {
    return std::nullopt;
  }
  return fail();
}

} // namespace charset::utf16
