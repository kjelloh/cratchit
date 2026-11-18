#pragma once

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit {

// One `path = value` line of a cratchit data file
struct PathValue {
  std::string path;
  std::string value;
};

using PathValues = std::vector<PathValue>;

// Parse `path = value` lines, in file order. Both sides are trimmed, so a value
// may contain spaces. Blank lines are skipped. Error text for a line without
// `=`, an empty path, or a path given twice.
std::expected<PathValues, std::string> parse_path_values(std::string_view text);

// The value of a path, if present
std::optional<std::string> find_value(PathValues const& entries, std::string_view path);

// Replace the value of a path, or add it last when missing
void set_value(PathValues& entries, std::string_view path, std::string_view value);

// One `path = value` line per entry, in order
std::string format_path_values(PathValues const& entries);

// Read and parse a file. Error text names the file.
std::expected<PathValues, std::string> read_path_values(std::filesystem::path const& file);

// Write entries to a file, replacing what it held. Error text names the file.
std::expected<void, std::string> write_path_values(std::filesystem::path const& file, PathValues const& entries);

} // namespace cratchit
