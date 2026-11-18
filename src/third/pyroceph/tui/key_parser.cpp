#include "pyroceph/tui/key_parser.hpp"

#include <string>
#include <utility>

namespace pyroceph::tui {

namespace {

constexpr CodePoint esc = 0x1B;

// ESC [ A and ESC O A and so on
std::optional<FunctionKey> letter_key(CodePoint final_byte) {
  switch (final_byte) {
  case U'A': return FunctionKey::up;
  case U'B': return FunctionKey::down;
  case U'C': return FunctionKey::right;
  case U'D': return FunctionKey::left;
  case U'H': return FunctionKey::home;
  case U'F': return FunctionKey::end;
  default: return std::nullopt;
  }
}

// ESC [ 3 ~ and so on
std::optional<FunctionKey> tilde_key(std::u32string const& parameters) {
  auto const number = parameters.substr(0, parameters.find(U';'));
  if (number == U"1" || number == U"7") return FunctionKey::home;
  if (number == U"4" || number == U"8") return FunctionKey::end;
  if (number == U"3") return FunctionKey::del;
  if (number == U"5") return FunctionKey::page_up;
  if (number == U"6") return FunctionKey::page_down;
  return std::nullopt;
}

bool is_sequence_character(CodePoint code_point) { return code_point >= 0x20 && code_point <= 0x7E; }

} // namespace

std::optional<CodePoint> KeyParser::next_code_point() {
  if (pending_) return std::exchange(pending_, std::nullopt);
  return reader_.next();
}

std::optional<FunctionKey> KeyParser::control_sequence() {
  // Parameter (0x30-0x3F) and intermediate (0x20-0x2F) characters, then one
  // final character (0x40-0x7E)
  std::u32string parameters;
  while (auto const code_point = reader_.next()) {
    if (!is_sequence_character(*code_point)) {
      pending_ = code_point; // broken sequence: drop it, keep the key that broke it
      return std::nullopt;
    }
    if (*code_point >= 0x40) {
      if (*code_point == U'~') return tilde_key(parameters);
      return letter_key(*code_point); // modifiers, as in ESC [ 1 ; 5 A, are ignored
    }
    parameters.push_back(*code_point);
  }
  return std::nullopt;
}

std::optional<Key> KeyParser::next() {
  while (auto const code_point = next_code_point()) {
    if (*code_point != esc) return Key{*code_point};
    if (!reader_.ready(escape_wait)) return FunctionKey::escape;
    auto const second = reader_.next();
    if (!second) return FunctionKey::escape;
    if (*second == U'[') {
      if (auto const key = control_sequence()) return *key;
      continue; // unknown or broken: dropped
    }
    if (*second == U'O') {
      auto const third = reader_.next();
      if (!third) return std::nullopt;
      if (auto const key = letter_key(*third)) return *key;
      if (!is_sequence_character(*third)) pending_ = third;
      continue; // e.g. F1 (ESC O P): dropped
    }
    // ESC then a key of its own: Esc was pressed, and the key comes next
    pending_ = second;
    return FunctionKey::escape;
  }
  return std::nullopt;
}

} // namespace pyroceph::tui
