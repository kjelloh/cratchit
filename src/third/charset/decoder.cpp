#include "charset/decoder.hpp"

namespace charset {

AnyDecoder make_any_decoder(Encoding encoding) {
  switch (encoding) {
  case Encoding::utf8: return utf8::Decoder();
  case Encoding::utf16be: return utf16::Decoder(utf16::ByteOrder::big_endian);
  case Encoding::utf16le: return utf16::Decoder(utf16::ByteOrder::little_endian);
  case Encoding::cp437: return cp437::Decoder();
  }
  return utf8::Decoder();
}

std::optional<unicode::CodePoint> Decoder::push(std::uint8_t byte) {
  return std::visit([byte](auto& decoder) { return decoder.push(byte); }, decoder_);
}

std::optional<unicode::CodePoint> Decoder::finish() {
  return std::visit(
      [](auto& decoder) -> std::optional<unicode::CodePoint> {
        // else, so that the other return is discarded rather than unreachable (MSVC C4702)
        if constexpr (requires { decoder.finish(); }) {
          return decoder.finish();
        } else {
          return std::nullopt;
        }
      },
      decoder_);
}

std::size_t Decoder::failure_count() const {
  return std::visit([](auto const& decoder) { return decoder.failure_count(); }, decoder_);
}

} // namespace charset
