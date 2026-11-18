#include "charset/transcoder.hpp"

#include "charset/decoder.hpp"

#include <variant>

namespace charset {

namespace {

using AnyEncoder = std::variant<utf8::Encoder, utf16::Encoder, cp437::Encoder>;

AnyEncoder make_encoder(Encoding encoding) {
  switch (encoding) {
  case Encoding::utf8: return utf8::Encoder();
  case Encoding::utf16be: return utf16::Encoder(utf16::ByteOrder::big_endian);
  case Encoding::utf16le: return utf16::Encoder(utf16::ByteOrder::little_endian);
  case Encoding::cp437: return cp437::Encoder();
  }
  return utf8::Encoder();
}

} // namespace

Transcoder make_transcoder(Encoding from, Encoding to) {
  return std::visit(
      [](auto decoder, auto encoder) {
        return Transcoder(BasicTranscoder(std::move(decoder), std::move(encoder)));
      },
      make_any_decoder(from), make_encoder(to));
}

} // namespace charset
