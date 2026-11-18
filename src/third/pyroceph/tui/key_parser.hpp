#pragma once

#include "pyroceph/tui/key_reader.hpp"

#include <chrono>
#include <optional>
#include <variant>

namespace pyroceph::tui {

// A key that gives no character
enum class FunctionKey { up, down, left, right, home, end, del, page_up, page_down, escape };

// One key press: a character, or a key that gives no character
using Key = std::variant<CodePoint, FunctionKey>;

// How long to wait after ESC for the rest of an escape sequence. Nothing
// within the wait means the user pressed Esc.
inline constexpr std::chrono::milliseconds escape_wait{50};

// Turns code points into keys. Knows the escape sequences terminals send for
// arrows, Home, End, Delete, Page Up and Page Down (ESC [ ... and ESC O ...).
// Other escape sequences, such as F-keys, are read to their end and dropped,
// so none of their characters come through.
class KeyParser {
public:
  explicit KeyParser(KeyReader& reader) : reader_(reader) {}

  // The next key, or nothing when the input has ended
  std::optional<Key> next();

private:
  std::optional<CodePoint> next_code_point();

  // The key of ESC [ ..., or nothing for a sequence it does not know
  std::optional<FunctionKey> control_sequence();

  KeyReader& reader_;
  std::optional<CodePoint> pending_; // read ahead, not part of a sequence
};

} // namespace pyroceph::tui
