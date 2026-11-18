#pragma once

#include <optional>

namespace pyroceph::tui {

// The width in columns of the terminal the program writes to (stdout), or
// nothing when stdout is no terminal (e.g. a pipe or a file). Asked anew each
// time, so it follows when the user resizes the window. On Windows also
// nothing when the console does not follow escape sequences.
std::optional<int> terminal_width();

} // namespace pyroceph::tui
