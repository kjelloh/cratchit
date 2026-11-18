#include "pyroceph/tui/terminal_width.hpp"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace pyroceph::tui {

std::optional<int> terminal_width() {
#if defined(_WIN32)
  // The width is only of use when the console follows escape sequences
  auto const out = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;
  if (!GetConsoleMode(out, &mode) || (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) == 0) return std::nullopt;
  CONSOLE_SCREEN_BUFFER_INFO info{};
  if (!GetConsoleScreenBufferInfo(out, &info)) return std::nullopt;
  int const width = info.srWindow.Right - info.srWindow.Left + 1;
#else
  if (!isatty(STDOUT_FILENO)) return std::nullopt;
  winsize size{};
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) != 0) return std::nullopt;
  int const width = size.ws_col;
#endif
  if (width <= 0) return std::nullopt;
  return width;
}

} // namespace pyroceph::tui
