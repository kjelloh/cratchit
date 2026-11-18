#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <string>

namespace cratchit {

// SHA-256 (FIPS 180-4), over bytes added a piece at a time. Identifies a file
// by its content, e.g. an SIE file in the inventory, whatever its name and place.
class Sha256 {
public:
  Sha256();
  void add(std::span<std::uint8_t const> bytes);
  // The digest as 64 lowercase hex digits. Ends the hashing: add nothing after it.
  std::string hex();

private:
  void block(std::uint8_t const* data);
  std::array<std::uint32_t, 8> state_;
  std::array<std::uint8_t, 64> buffer_{};
  std::size_t buffered_ = 0;
  std::uint64_t length_ = 0; // bytes added
};

// The SHA-256 of a file's bytes, as 64 lowercase hex digits. Error text when it cannot be read.
std::expected<std::string, std::string> sha256_of_file(std::filesystem::path const& file);

} // namespace cratchit
