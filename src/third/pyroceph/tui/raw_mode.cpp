#include "pyroceph/tui/raw_mode.hpp"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_INPUT
#define ENABLE_VIRTUAL_TERMINAL_INPUT 0x0200
#endif
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace pyroceph::tui {

#if defined(_WIN32)

struct RawMode::Saved {
  HANDLE input;
  DWORD mode;
};

RawMode::RawMode() {
  auto const input = GetStdHandle(STD_INPUT_HANDLE);
  DWORD mode = 0;
  if (input == INVALID_HANDLE_VALUE || !GetConsoleMode(input, &mode)) return;
  auto const raw = mode & ~static_cast<DWORD>(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
  // Arrows and other keys as escape sequences, as on macOS and Linux (Windows 10
  // and later). Without it they give no characters at all.
  if (!SetConsoleMode(input, raw | ENABLE_VIRTUAL_TERMINAL_INPUT) && !SetConsoleMode(input, raw)) return;
  saved_ = std::make_unique<Saved>(Saved{input, mode});
}

RawMode::~RawMode() {
  if (saved_) SetConsoleMode(saved_->input, saved_->mode);
}

#else

struct RawMode::Saved {
  termios mode;
};

RawMode::RawMode() {
  if (!isatty(STDIN_FILENO)) return;
  termios mode{};
  if (tcgetattr(STDIN_FILENO, &mode) != 0) return;
  auto raw = mode;
  raw.c_lflag &= ~static_cast<tcflag_t>(ICANON | ECHO | ISIG);
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;
  if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) return;
  saved_ = std::make_unique<Saved>(Saved{mode});
}

RawMode::~RawMode() {
  if (saved_) tcsetattr(STDIN_FILENO, TCSANOW, &saved_->mode);
}

#endif

bool RawMode::active() const { return saved_ != nullptr; }

} // namespace pyroceph::tui
