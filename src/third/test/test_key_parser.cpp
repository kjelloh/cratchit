#include "pyroceph/tui/key_parser.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

using charset::Encoding;
using pyroceph::tui::ByteReady;
using pyroceph::tui::FunctionKey;
using pyroceph::tui::Key;
using pyroceph::tui::KeyParser;
using pyroceph::tui::CodePoint;
using pyroceph::tui::KeyReader;

namespace {

// A reader of the bytes. `ready` tells whether the next byte is there at once,
// as when a terminal sends a whole sequence, or only later, as after a lone Esc.
KeyReader reader_of(std::vector<std::uint8_t> bytes, ByteReady ready = {}) {
  auto shared = std::make_shared<std::vector<std::uint8_t>>(std::move(bytes));
  auto next = std::make_shared<std::size_t>(0);
  return KeyReader(
      [shared, next]() -> std::optional<std::uint8_t> {
        if (*next == shared->size()) return std::nullopt;
        return (*shared)[(*next)++];
      },
      Encoding::utf8, std::move(ready));
}

std::vector<Key> parse_all(KeyReader& reader) {
  KeyParser parser(reader);
  std::vector<Key> keys;
  while (auto key = parser.next()) keys.push_back(*key);
  return keys;
}

std::vector<Key> parse_all(std::vector<std::uint8_t> bytes) {
  auto reader = reader_of(std::move(bytes));
  return parse_all(reader);
}

} // namespace

TEST(KeyParser, CharactersAreCodePoints) {
  EXPECT_EQ(parse_all({'a', 0xC3, 0xA5}), (std::vector<Key>{U'a', U'å'}));
}

TEST(KeyParser, Arrows) {
  EXPECT_EQ(parse_all({0x1B, '[', 'A', 0x1B, '[', 'B', 0x1B, '[', 'C', 0x1B, '[', 'D'}),
            (std::vector<Key>{FunctionKey::up, FunctionKey::down, FunctionKey::right, FunctionKey::left}));
}

TEST(KeyParser, ArrowsInApplicationMode) {
  EXPECT_EQ(parse_all({0x1B, 'O', 'A', 0x1B, 'O', 'D'}), (std::vector<Key>{FunctionKey::up, FunctionKey::left}));
}

TEST(KeyParser, ArrowWithModifierIsTheArrow) {
  EXPECT_EQ(parse_all({0x1B, '[', '1', ';', '5', 'C'}), (std::vector<Key>{FunctionKey::right})); // Ctrl-right
}

TEST(KeyParser, HomeEndDeletePage) {
  EXPECT_EQ(parse_all({0x1B, '[', 'H', 0x1B, '[', 'F', 0x1B, '[', '1', '~', 0x1B, '[', '4', '~', 0x1B, '[', '3', '~',
                       0x1B, '[', '5', '~', 0x1B, '[', '6', '~'}),
            (std::vector<Key>{FunctionKey::home, FunctionKey::end, FunctionKey::home, FunctionKey::end,
                              FunctionKey::del, FunctionKey::page_up, FunctionKey::page_down}));
}

TEST(KeyParser, UnknownSequencesAreDroppedWhole) {
  // F5 (ESC [ 1 5 ~), F1 (ESC O P), an unknown final letter (ESC [ 2 Z)
  EXPECT_EQ(parse_all({'a', 0x1B, '[', '1', '5', '~', 0x1B, 'O', 'P', 0x1B, '[', '2', 'Z', 'b'}),
            (std::vector<Key>{U'a', U'b'}));
}

TEST(KeyParser, BrokenSequenceKeepsTheKeyThatBrokeIt) {
  EXPECT_EQ(parse_all({0x1B, '[', '1', 0x04}), (std::vector<Key>{CodePoint{0x04}}));
}

TEST(KeyParser, EscFollowedByACharacterIsEscThenTheCharacter) {
  EXPECT_EQ(parse_all({0x1B, 'x'}), (std::vector<Key>{FunctionKey::escape, U'x'}));
}

TEST(KeyParser, EscAtTheEndIsEsc) {
  EXPECT_EQ(parse_all({'a', 0x1B}), (std::vector<Key>{U'a', FunctionKey::escape}));
}

TEST(KeyParser, TwoEscsAreTwoEscs) {
  EXPECT_EQ(parse_all({0x1B, 0x1B}), (std::vector<Key>{FunctionKey::escape, FunctionKey::escape}));
}

TEST(KeyParser, EscWithNothingWithinTheWaitIsEsc) {
  // The user pressed Esc, then later [ and A by hand
  auto reader = reader_of({0x1B, '[', 'A'}, [](std::chrono::milliseconds) { return false; });
  EXPECT_EQ(parse_all(reader), (std::vector<Key>{FunctionKey::escape, U'[', U'A'}));
}
