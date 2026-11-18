#include "charset/detect.hpp"

#include "charset/utf8.hpp"

namespace charset::detect {

bool is_utf8(std::span<std::uint8_t const> bytes) {
  utf8::Decoder decoder;
  bool multi_byte = false;
  for (auto const byte : bytes) {
    if (auto const code_point = decoder.push(byte); code_point && *code_point >= 0x80) {
      multi_byte = true;
    }
  }
  decoder.finish();
  return multi_byte && decoder.failure_count() == 0;
}

} // namespace charset::detect
