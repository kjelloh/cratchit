#pragma once

#include <memory>

namespace pyroceph::tui {

// While alive, the terminal gives each key at once: no waiting for Enter, no
// echo, and Ctrl-C and Ctrl-D come as characters (U+0003, U+0004) instead of
// stopping the program or ending the input. That way the terminal is always
// set back when this is destroyed. Does nothing when stdin is not a terminal.
// On Windows it also asks the console for escape sequences for arrows and
// other keys, as terminals on macOS and Linux send.
class RawMode {
public:
  RawMode();
  ~RawMode();
  RawMode(RawMode const&) = delete;
  RawMode& operator=(RawMode const&) = delete;

  // True when the terminal was switched and will be set back
  bool active() const;

private:
  struct Saved;
  std::unique_ptr<Saved> saved_;
};

} // namespace pyroceph::tui
