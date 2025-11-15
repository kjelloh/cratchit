#include "RuntimeEncoding.hpp"
#include "text/encoding.hpp"
#include "logger/log.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

// RuntimeEncoding Implementation

RuntimeEncoding::RuntimeEncoding(DetectedEncoding const& detected_encoing)
  : m_detected_encoding{detected_encoing} {}

RuntimeEncoding::DetectedEncoding RuntimeEncoding::detected_encoding() {
  return m_detected_encoding;
}

std::string RuntimeEncoding::get_encoding_display_name() {
  return text::encoding::enum_to_display_name(m_detected_encoding);
}

RuntimeEncoding to_inferred_runtime_encoding() {
  // Cross-platform runtime encoding detection
  
#ifdef __APPLE__
  // macOS: Terminal.app and most terminals default to UTF-8
  auto encoding = RuntimeEncoding::DetectedEncoding::UTF8;
  spdlog::info("RuntimeEncoding: macOS detected - using UTF-8");
  return RuntimeEncoding(encoding);
  
#elif defined(_WIN32)
  // Windows: Check console code page (for narrow char I/O)
  // Note: Windows internally uses UTF-16, but console I/O depends on code page
  UINT cp = GetConsoleOutputCP();
  auto encoding = RuntimeEncoding::DetectedEncoding::Unknown;
  
  switch (cp) {
    case 65001: // UTF-8 (modern Windows Terminal, WSL)
      encoding = RuntimeEncoding::DetectedEncoding::UTF8;
      spdlog::info("RuntimeEncoding: Windows UTF-8 console (CP65001) detected");
      break;
    case 1252: // Windows-1252 (Western European)
      encoding = RuntimeEncoding::DetectedEncoding::WINDOWS_1252;
      spdlog::info("RuntimeEncoding: Windows-1252 console (CP1252) detected");
      break;
    case 437: // CP437 (legacy DOS)
      encoding = RuntimeEncoding::DetectedEncoding::CP437;
      spdlog::info("RuntimeEncoding: CP437 console detected");
      break;
    default:
      encoding = RuntimeEncoding::DetectedEncoding::Unknown;
      spdlog::info("RuntimeEncoding: Windows unknown console code page {}", cp);
      break;
  }
  return RuntimeEncoding(encoding);
  
#elif defined(__linux__)
  // Linux: Modern distributions default to UTF-8
  auto encoding = RuntimeEncoding::DetectedEncoding::UTF8;
  spdlog::info("RuntimeEncoding: Linux detected - assuming UTF-8");
  return RuntimeEncoding(encoding);
  
#else
  // Other POSIX systems
  auto encoding = RuntimeEncoding::DetectedEncoding::Unknown;
  spdlog::info("RuntimeEncoding: Unknown platform - returning Unknown encoding");
  return RuntimeEncoding(encoding);
#endif
}
