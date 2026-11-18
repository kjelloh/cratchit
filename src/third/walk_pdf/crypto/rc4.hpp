#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace crypto {

// RC4 stream cipher. Not for security: PDF's Standard security handler (V 1
// and 2) encrypts strings and streams with it. Encrypting and decrypting are
// the same operation.
class Rc4 {
public:
  explicit Rc4(std::string_view key);

  // The bytes XORed with the next bytes of the key stream
  std::string apply(std::string_view bytes);

private:
  std::array<std::uint8_t, 256> state_{};
  std::uint8_t i_ = 0;
  std::uint8_t j_ = 0;
};

// Bytes encrypted or decrypted with key, in one go
std::string rc4(std::string_view key, std::string_view bytes);

} // namespace crypto
