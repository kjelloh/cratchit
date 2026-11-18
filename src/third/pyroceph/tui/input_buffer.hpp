#pragma once

#include "pyroceph/tui/key_reader.hpp"

#include <cstddef>
#include <string>

namespace pyroceph::tui {

// True for the C0 and C1 control characters (U+0000-U+001F, U+007F-U+009F),
// which never go into an InputBuffer
bool is_control(CodePoint code_point);

// The code points the user has typed but not yet entered
class InputBuffer {
public:
  bool empty() const { return text_.empty(); }
  std::size_t size() const { return text_.size(); }
  std::u32string const& text() const { return text_; }

  // Adds a code point at the end. A control character is ignored. True when added.
  bool push(CodePoint code_point);

  // Removes the last code point. False when already empty.
  bool erase();

  void clear() { text_.clear(); }

private:
  std::u32string text_;
};

} // namespace pyroceph::tui
