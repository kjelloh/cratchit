#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace crypto {

// MD5 (RFC 1321). Not for security: PDF's Standard security handler uses it
// to derive keys.
using Md5Digest = std::array<std::uint8_t, 16>;

class Md5 {
public:
  Md5();

  // Adds bytes to the message
  Md5& update(std::string_view bytes);

  // The digest of everything added. The Md5 can not be used afterwards.
  Md5Digest finish();

private:
  void block(std::uint8_t const* bytes);

  std::array<std::uint32_t, 4> state_;
  std::array<std::uint8_t, 64> buffer_{};
  std::size_t buffered_ = 0;
  std::uint64_t length_ = 0; // in bytes
};

// The digest of bytes, in one go
Md5Digest md5(std::string_view bytes);

} // namespace crypto
