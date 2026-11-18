#include "core/sha256.hpp"

#include <bit>
#include <format>
#include <fstream>
#include <vector>

namespace cratchit {

namespace {

constexpr std::array<std::uint32_t, 64> k{
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

} // namespace

Sha256::Sha256()
    : state_{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19} {}

void Sha256::block(std::uint8_t const* data) {
  std::array<std::uint32_t, 64> w{};
  for (std::size_t i = 0; i < 16; ++i) {
    w[i] = (std::uint32_t{data[4 * i]} << 24) | (std::uint32_t{data[4 * i + 1]} << 16) |
           (std::uint32_t{data[4 * i + 2]} << 8) | std::uint32_t{data[4 * i + 3]};
  }
  for (std::size_t i = 16; i < 64; ++i) {
    auto const s0 = std::rotr(w[i - 15], 7) ^ std::rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
    auto const s1 = std::rotr(w[i - 2], 17) ^ std::rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
    w[i] = w[i - 16] + s0 + w[i - 7] + s1;
  }
  auto [a, b, c, d, e, f, g, h] = state_;
  for (std::size_t i = 0; i < 64; ++i) {
    auto const s1 = std::rotr(e, 6) ^ std::rotr(e, 11) ^ std::rotr(e, 25);
    auto const choice = (e & f) ^ (~e & g);
    auto const t1 = h + s1 + choice + k[i] + w[i];
    auto const s0 = std::rotr(a, 2) ^ std::rotr(a, 13) ^ std::rotr(a, 22);
    auto const majority = (a & b) ^ (a & c) ^ (b & c);
    auto const t2 = s0 + majority;
    h = g;
    g = f;
    f = e;
    e = d + t1;
    d = c;
    c = b;
    b = a;
    a = t1 + t2;
  }
  state_[0] += a;
  state_[1] += b;
  state_[2] += c;
  state_[3] += d;
  state_[4] += e;
  state_[5] += f;
  state_[6] += g;
  state_[7] += h;
}

void Sha256::add(std::span<std::uint8_t const> bytes) {
  length_ += bytes.size();
  for (auto const byte : bytes) {
    buffer_[buffered_++] = byte;
    if (buffered_ == buffer_.size()) {
      block(buffer_.data());
      buffered_ = 0;
    }
  }
}

std::string Sha256::hex() {
  auto const bits = length_ * 8;
  std::uint8_t const one = 0x80;
  add({&one, 1});
  std::uint8_t const zero = 0;
  while (buffered_ != 56) add({&zero, 1});
  std::array<std::uint8_t, 8> length{};
  for (std::size_t i = 0; i < 8; ++i) length[i] = static_cast<std::uint8_t>(bits >> (56 - 8 * i));
  add(length);
  std::string result;
  for (auto const word : state_) result += std::format("{:08x}", word);
  return result;
}

std::expected<std::string, std::string> sha256_of_file(std::filesystem::path const& file) {
  std::ifstream in(file, std::ios::binary);
  if (!in) return std::unexpected(std::format("Kan inte läsa {}", file.string()));
  Sha256 hash;
  std::vector<char> buffer(1 << 16);
  while (in) {
    in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    auto const count = static_cast<std::size_t>(in.gcount());
    hash.add({reinterpret_cast<std::uint8_t const*>(buffer.data()), count});
  }
  if (in.bad()) return std::unexpected(std::format("Kan inte läsa {}", file.string()));
  return hash.hex();
}

} // namespace cratchit
