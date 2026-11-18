#pragma once

#include <expected>
#include <filesystem>
#include <format>
#include <source_location>
#include <string>
#include <string_view>

// The logging facade. Every event is one line in the log file, never on screen:
//
//   LOG_BUSINESS("Opened workspace {}", folder.string());
//
// LOG_BUSINESS              a normal app event, as information for the user
// LOG_DEVELOPMENT_TRACE     tracing during development; compiled in only when
//                           CRATCHIT_DEVELOPMENT_TRACE is 1 (see CMakeLists.txt)
// LOG_DESIGN_INSUFFICIENCY  something happened that cratchit is not designed for
//
// The arguments are those of std::format.

#define LOG_BUSINESS(...)                                                                                          \
  ::cratchit::log::write(::cratchit::log::Kind::Business, std::source_location::current(), std::format(__VA_ARGS__))

#define LOG_DESIGN_INSUFFICIENCY(...)                                                                              \
  ::cratchit::log::write(::cratchit::log::Kind::DesignInsufficiency, std::source_location::current(),               \
                         std::format(__VA_ARGS__))

#if CRATCHIT_DEVELOPMENT_TRACE
#define LOG_DEVELOPMENT_TRACE(...)                                                                                 \
  ::cratchit::log::write(::cratchit::log::Kind::DevelopmentTrace, std::source_location::current(),                  \
                         std::format(__VA_ARGS__))
#else
// Still checked by the compiler, so a trace can't rot, but never run
#define LOG_DEVELOPMENT_TRACE(...)                                                                                 \
  do {                                                                                                             \
    if constexpr (false) {                                                                                         \
      (void)std::format(__VA_ARGS__);                                                                              \
    }                                                                                                              \
  } while (false)
#endif

namespace cratchit::log {

enum class Kind { Business, DevelopmentTrace, DesignInsufficiency };

// Appends one event to the log. Until open() is called, events are kept in
// memory and written when it is.
void write(Kind kind, std::source_location where, std::string_view message);

// Appends from now on to file, creating it if missing, after any events kept
// so far. Replaces a log opened before. Error text if the file can't be opened;
// events are then kept in memory.
std::expected<void, std::string> open(std::filesystem::path const& file);

// Stops writing to the file; later events are kept in memory again
void close();

// The line write() appends, without the newline:
// `<YYYY-MM-DDTHH:MM:SS> <KIND> <file>:<line> <message>`, newlines in message as \n
std::string format_line(std::string_view time, Kind kind, std::source_location where, std::string_view message);

} // namespace cratchit::log
