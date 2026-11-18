#include "core/log.hpp"

#include <ctime>
#include <fstream>
#include <mutex>
#include <vector>

namespace cratchit::log {

namespace {

std::mutex mutex;
std::ofstream file_out;
std::vector<std::string> kept; // events written before the log is open

std::string_view name_of(Kind kind) {
  switch (kind) {
  case Kind::Business:
    return "BUSINESS";
  case Kind::DevelopmentTrace:
    return "DEVELOPMENT_TRACE";
  case Kind::DesignInsufficiency:
    return "DESIGN_INSUFFICIENCY";
  }
  return "UNKNOWN";
}

std::string now_local() {
  std::time_t const now = std::time(nullptr);
  std::tm local{};
#ifdef _WIN32
  localtime_s(&local, &now);
#else
  localtime_r(&now, &local);
#endif
  return std::format("{:04}-{:02}-{:02}T{:02}:{:02}:{:02}", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
                     local.tm_hour, local.tm_min, local.tm_sec);
}

} // namespace

std::string format_line(std::string_view time, Kind kind, std::source_location where, std::string_view message) {
  auto const file = std::filesystem::path(where.file_name()).filename().string();
  auto line = std::format("{} {} {}:{} ", time, name_of(kind), file, where.line());
  for (char c : message) {
    if (c == '\n') {
      line += "\\n";
    } else if (c != '\r') {
      line += c;
    }
  }
  return line;
}

void write(Kind kind, std::source_location where, std::string_view message) {
  auto line = format_line(now_local(), kind, where, message);
  std::lock_guard lock(mutex);
  if (file_out.is_open()) {
    file_out << line << '\n';
    file_out.flush();
  } else {
    kept.push_back(std::move(line));
  }
}

std::expected<void, std::string> open(std::filesystem::path const& file) {
  std::lock_guard lock(mutex);
  file_out.close();
  file_out.clear();
  file_out.open(file, std::ios::app);
  if (!file_out.is_open()) {
    return std::unexpected(std::format("Kan inte öppna loggen {}", file.string()));
  }
  for (auto const& line : kept) {
    file_out << line << '\n';
  }
  kept.clear();
  file_out.flush();
  return {};
}

void close() {
  std::lock_guard lock(mutex);
  file_out.close();
}

} // namespace cratchit::log
