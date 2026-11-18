#include "core/version.hpp"

#include <format>

namespace cratchit {

std::string_view version() {
  return CRATCHIT_VERSION;
}

std::vector<std::string> startup_lines(std::string_view version, std::string_view build_type,
                                       std::string_view build_time,
                                       std::optional<std::filesystem::path> const& program) {
  std::vector<std::string> lines;
  lines.push_back(std::format("Byggd: {} ({})", build_time, build_type));
  if (program) lines.push_back(std::format("Från: {}", program->string()));
  lines.push_back(std::format("Version: {}", version));
  return lines;
}

} // namespace cratchit
