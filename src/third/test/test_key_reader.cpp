#include "pyroceph/tui/key_reader.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

using charset::Encoding;
using pyroceph::tui::ByteSource;
using pyroceph::tui::CodePoint;
using pyroceph::tui::KeyReader;

namespace {

// Gives the bytes one at a time, then nothing
ByteSource bytes_of(std::vector<std::uint8_t> bytes) {
  auto shared = std::make_shared<std::vector<std::uint8_t>>(std::move(bytes));
  auto next = std::make_shared<std::size_t>(0);
  return [shared, next]() -> std::optional<std::uint8_t> {
    if (*next == shared->size()) return std::nullopt;
    return (*shared)[(*next)++];
  };
}

std::vector<CodePoint> read_all(KeyReader& keys) {
  std::vector<CodePoint> result;
  while (auto code_point = keys.next()) result.push_back(*code_point);
  return result;
}

} // namespace

TEST(KeyReader, Utf8OneCodePointPerKey) {
  KeyReader keys(bytes_of({'a', 0xC3, 0xA5, 0xC3, 0xA4, 0xC3, 0xB6}), Encoding::utf8);
  EXPECT_EQ(read_all(keys), (std::vector<CodePoint>{U'a', U'å', U'ä', U'ö'}));
  EXPECT_EQ(keys.failure_count(), 0U);
}

TEST(KeyReader, Cp437) {
  KeyReader keys(bytes_of({'a', 0x86, 0x84, 0x94}), Encoding::cp437);
  EXPECT_EQ(read_all(keys), (std::vector<CodePoint>{U'a', U'å', U'ä', U'ö'}));
}

TEST(KeyReader, Utf16leAsFromWindowsConsole) {
  KeyReader keys(bytes_of({'a', 0x00, 0xE5, 0x00}), Encoding::utf16le);
  EXPECT_EQ(read_all(keys), (std::vector<CodePoint>{U'a', U'å'}));
}

TEST(KeyReader, BrokenBytesGiveReplacementAndAreCounted) {
  KeyReader keys(bytes_of({'a', 0xFF, 'b', 0xC3}), Encoding::utf8);
  auto const replacement = charset::unicode::replacement_character;
  EXPECT_EQ(read_all(keys), (std::vector<CodePoint>{U'a', replacement, U'b', replacement}));
  EXPECT_EQ(keys.failure_count(), 2U);
}

TEST(KeyReader, EndedInputStaysEnded) {
  KeyReader keys(bytes_of({}), Encoding::utf8);
  EXPECT_FALSE(keys.next());
  EXPECT_FALSE(keys.next());
}

TEST(KeyReader, EscapeSequenceComesAsSeparateCodePoints) {
  KeyReader keys(bytes_of({0x1B, '[', 'A'}), Encoding::utf8); // arrow up
  EXPECT_EQ(read_all(keys), (std::vector<CodePoint>{0x1B, U'[', U'A'}));
}
