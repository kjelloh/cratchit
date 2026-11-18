#include "crypto/md5.hpp"

#include <bit>
#include <string>

namespace crypto {

namespace {

// RFC 1321, 3.4: shift amounts and T[i] = floor(abs(sin(i + 1)) * 2^32)
constexpr std::array<int, 64> shifts{7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                                     5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20,
                                     4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                                     6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

constexpr std::array<std::uint32_t, 64> table{
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

} // namespace

Md5::Md5() : state_{0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476} {}

void Md5::block(std::uint8_t const* bytes) {
  std::array<std::uint32_t, 16> words{};
  for (std::size_t i = 0; i < 16; ++i) {
    words[i] = static_cast<std::uint32_t>(bytes[4 * i]) |
               static_cast<std::uint32_t>(bytes[4 * i + 1]) << 8 |
               static_cast<std::uint32_t>(bytes[4 * i + 2]) << 16 |
               static_cast<std::uint32_t>(bytes[4 * i + 3]) << 24;
  }
  auto [a, b, c, d] = state_;
  for (std::size_t i = 0; i < 64; ++i) {
    std::uint32_t f = 0;
    std::size_t g = 0;
    if (i < 16) {
      f = (b & c) | (~b & d);
      g = i;
    } else if (i < 32) {
      f = (d & b) | (~d & c);
      g = (5 * i + 1) % 16;
    } else if (i < 48) {
      f = b ^ c ^ d;
      g = (3 * i + 5) % 16;
    } else {
      f = c ^ (b | ~d);
      g = (7 * i) % 16;
    }
    auto const rotated = std::rotl(a + f + table[i] + words[g], shifts[i]);
    a = d;
    d = c;
    c = b;
    b = b + rotated;
  }
  state_[0] += a;
  state_[1] += b;
  state_[2] += c;
  state_[3] += d;
}

Md5& Md5::update(std::string_view bytes) {
  length_ += bytes.size();
  for (auto const c : bytes) {
    buffer_[buffered_++] = static_cast<std::uint8_t>(c);
    if (buffered_ == buffer_.size()) {
      block(buffer_.data());
      buffered_ = 0;
    }
  }
  return *this;
}

Md5Digest Md5::finish() {
  // Pad with 0x80, zeros, and the length in bits, to a multiple of 64 bytes
  auto const bits = length_ * 8;
  std::string padding(1, '\x80');
  padding.append((buffered_ < 56 ? 55 : 119) - buffered_, '\0');
  for (int i = 0; i < 8; ++i) {
    padding += static_cast<char>((bits >> (8 * i)) & 0xFF);
  }
  update(padding);
  Md5Digest digest{};
  for (std::size_t i = 0; i < 16; ++i) {
    digest[i] = static_cast<std::uint8_t>(state_[i / 4] >> (8 * (i % 4)));
  }
  return digest;
}

Md5Digest md5(std::string_view bytes) { return Md5{}.update(bytes).finish(); }

} // namespace crypto
