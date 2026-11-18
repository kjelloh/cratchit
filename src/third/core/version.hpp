#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit {

// The application version, as set by the project() call in CMakeLists.txt
std::string_view version();

// The lines cratchit shows first when it starts, so that the user sees which
// build runs, one fact per line:
//   Byggd: 14:32:05 2026-11-15 (Debug)
//   Från: <program>
//   Version: 0.1.0
// Without a program file the line "Från: ..." is left out.
std::vector<std::string> startup_lines(std::string_view version, std::string_view build_type,
                                       std::string_view build_time,
                                       std::optional<std::filesystem::path> const& program);

} // namespace cratchit
