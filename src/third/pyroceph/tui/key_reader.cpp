#include "pyroceph/tui/key_reader.hpp"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <cerrno>
#include <poll.h>
#include <unistd.h>
#endif

#include <memory>
#include <utility>

namespace pyroceph::tui {

std::optional<CodePoint> KeyReader::next() {
  while (!ended_) {
    auto const byte = bytes_();
    if (!byte) {
      ended_ = true;
      return decoder_.finish();
    }
    if (auto const code_point = decoder_.push(*byte)) return code_point;
  }
  return std::nullopt;
}

namespace {

#if defined(_WIN32)

// The bytes of each UTF-16 unit from the console, low byte first
class ConsoleSource {
public:
  explicit ConsoleSource(HANDLE input) : input_(input) {}

  std::optional<std::uint8_t> operator()() {
    if (high_) return std::exchange(high_, std::nullopt);
    wchar_t unit = 0;
    DWORD read = 0;
    if (!ReadConsoleW(input_, &unit, 1, &read, nullptr) || read == 0) return std::nullopt;
    high_ = static_cast<std::uint8_t>(unit >> 8);
    return static_cast<std::uint8_t>(unit & 0xFF);
  }

  bool ready(std::chrono::milliseconds wait) const {
    if (high_) return true;
    return WaitForSingleObject(input_, static_cast<DWORD>(wait.count())) == WAIT_OBJECT_0;
  }

private:
  HANDLE input_;
  std::optional<std::uint8_t> high_;
};

// Raw bytes from a pipe or file
class FileSource {
public:
  explicit FileSource(HANDLE input) : input_(input) {}

  std::optional<std::uint8_t> operator()() {
    std::uint8_t byte = 0;
    DWORD read = 0;
    if (!ReadFile(input_, &byte, 1, &read, nullptr) || read == 0) return std::nullopt;
    return byte;
  }

private:
  HANDLE input_;
};

#else

std::optional<std::uint8_t> read_stdin_byte() {
  std::uint8_t byte = 0;
  while (true) {
    auto const read = ::read(STDIN_FILENO, &byte, 1);
    if (read == 1) return byte;
    if (read < 0 && errno == EINTR) continue;
    return std::nullopt;
  }
}

bool stdin_ready(std::chrono::milliseconds wait) {
  pollfd fd{STDIN_FILENO, POLLIN, 0};
  return ::poll(&fd, 1, static_cast<int>(wait.count())) != 0;
}

#endif

} // namespace

KeyReader terminal_key_reader(charset::Encoding encoding) {
#if defined(_WIN32)
  auto const input = GetStdHandle(STD_INPUT_HANDLE);
  DWORD mode = 0;
  if (GetConsoleMode(input, &mode)) {
    auto console = std::make_shared<ConsoleSource>(input);
    return KeyReader([console] { return (*console)(); }, charset::Encoding::utf16le,
                     [console](std::chrono::milliseconds wait) { return console->ready(wait); });
  }
  return KeyReader(FileSource(input), encoding);
#else
  return KeyReader(read_stdin_byte, encoding, stdin_ready);
#endif
}

} // namespace pyroceph::tui
