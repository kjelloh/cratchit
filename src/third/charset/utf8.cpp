#include "charset/utf8.hpp"

namespace charset::utf8 {

namespace {

// Lead byte bits ORed with the top bits, then continuation bytes 10xxxxxx
Bytes encode(unicode::CodePoint code_point, std::size_t size, CodePoint lead) {
  Bytes result;
  result.size = size;
  for (std::size_t i = size - 1; i > 0; --i) {
    result.bytes[i] = static_cast<CodePoint>(0x80 | (code_point & 0x3F));
    code_point >>= 6;
  }
  result.bytes[0] = static_cast<CodePoint>(lead | code_point);
  return result;
}

} // namespace

Bytes Encoder::push(unicode::CodePoint code_point) {
  if (code_point < 0x80) {
    return encode(code_point, 1, 0x00);
  }
  if (code_point < 0x800) {
    return encode(code_point, 2, 0xC0);
  }
  if ((code_point >= 0xD800 && code_point <= 0xDFFF) || code_point > 0x10FFFF) {
    ++unmapped_count_;
    return encode(unicode::replacement_character, 3, 0xE0);
  }
  if (code_point < 0x10000) {
    return encode(code_point, 3, 0xE0);
  }
  return encode(code_point, 4, 0xF0);
}

std::optional<unicode::CodePoint> Decoder::push(CodePoint byte) {
  if (pending_.size == 0) {
    lower_ = 0x80;
    upper_ = 0xBF;
    if (byte < 0x80) {
      return unicode::CodePoint{byte};
    }
    if (byte >= 0xC2 && byte <= 0xDF) {
      expected_size_ = 2;
      code_point_ = byte & 0x1F;
    } else if (byte >= 0xE0 && byte <= 0xEF) {
      expected_size_ = 3;
      code_point_ = byte & 0x0F;
      if (byte == 0xE0) {
        lower_ = 0xA0; // else overlong
      } else if (byte == 0xED) {
        upper_ = 0x9F; // else a surrogate
      }
    } else if (byte >= 0xF0 && byte <= 0xF4) {
      expected_size_ = 4;
      code_point_ = byte & 0x07;
      if (byte == 0xF0) {
        lower_ = 0x90; // else overlong
      } else if (byte == 0xF4) {
        upper_ = 0x8F; // else above U+10FFFF
      }
    } else {
      // A continuation byte, C0, C1 (always overlong) or F5-FF
      return fail(byte);
    }
    pending_.bytes[pending_.size++] = byte;
    return std::nullopt;
  }
  if (byte < lower_ || byte > upper_) {
    return fail(byte);
  }
  lower_ = 0x80;
  upper_ = 0xBF;
  pending_.bytes[pending_.size++] = byte;
  code_point_ = (code_point_ << 6) | (byte & 0x3F);
  if (pending_.size < expected_size_) {
    return std::nullopt;
  }
  pending_.size = 0;
  return code_point_;
}

std::optional<unicode::CodePoint> Decoder::finish() {
  if (pending_.size == 0) {
    return std::nullopt;
  }
  return fail(std::nullopt);
}

std::size_t Decoder::failure_count() const {
  return failure_count_;
}

std::vector<CodePoint> const& Decoder::failed_bytes() const {
  return failed_bytes_;
}

// Consumes the pending bytes and the byte that broke them, if any
unicode::CodePoint Decoder::fail(std::optional<CodePoint> breaking_byte) {
  failed_bytes_.insert(failed_bytes_.end(), pending_.begin(), pending_.end());
  if (breaking_byte) {
    failed_bytes_.push_back(*breaking_byte);
  }
  pending_.size = 0;
  ++failure_count_;
  return unicode::replacement_character;
}

} // namespace charset::utf8
