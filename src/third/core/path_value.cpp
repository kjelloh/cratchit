#include "core/path_value.hpp"

#include <algorithm>
#include <format>
#include <fstream>
#include <sstream>

namespace cratchit {

namespace {

std::string_view trim(std::string_view text) {
  auto const first = text.find_first_not_of(" \t\r");
  if (first == std::string_view::npos) {
    return {};
  }
  auto const last = text.find_last_not_of(" \t\r");
  return text.substr(first, last - first + 1);
}

} // namespace

std::expected<PathValues, std::string> parse_path_values(std::string_view text) {
  PathValues entries;
  std::size_t line_number = 0;
  while (!text.empty()) {
    ++line_number;
    auto const end = text.find('\n');
    auto const line = trim(text.substr(0, end));
    text = end == std::string_view::npos ? std::string_view{} : text.substr(end + 1);
    if (line.empty()) {
      continue;
    }
    auto const equals = line.find('=');
    if (equals == std::string_view::npos) {
      return std::unexpected(std::format("rad {}: förväntade 'path = value'", line_number));
    }
    auto const path = trim(line.substr(0, equals));
    if (path.empty()) {
      return std::unexpected(std::format("rad {}: ingen path före '='", line_number));
    }
    if (find_value(entries, path)) {
      return std::unexpected(std::format("rad {}: '{}' anges två gånger", line_number, path));
    }
    entries.push_back({std::string(path), std::string(trim(line.substr(equals + 1)))});
  }
  return entries;
}

std::optional<std::string> find_value(PathValues const& entries, std::string_view path) {
  auto const it = std::ranges::find(entries, path, &PathValue::path);
  if (it == entries.end()) {
    return std::nullopt;
  }
  return it->value;
}

void set_value(PathValues& entries, std::string_view path, std::string_view value) {
  auto const it = std::ranges::find(entries, path, &PathValue::path);
  if (it == entries.end()) {
    entries.push_back({std::string(path), std::string(value)});
  } else {
    it->value = value;
  }
}

std::string format_path_values(PathValues const& entries) {
  std::string text;
  for (auto const& entry : entries) {
    text += std::format("{} = {}\n", entry.path, entry.value);
  }
  return text;
}

std::expected<PathValues, std::string> read_path_values(std::filesystem::path const& file) {
  std::ifstream in(file);
  std::ostringstream text;
  text << in.rdbuf();
  if (!in) {
    return std::unexpected(std::format("Kan inte läsa {}", file.string()));
  }
  auto entries = parse_path_values(text.str());
  if (!entries) {
    return std::unexpected(std::format("{}: {}", file.string(), entries.error()));
  }
  return entries;
}

std::expected<void, std::string> write_path_values(std::filesystem::path const& file, PathValues const& entries) {
  std::ofstream out(file);
  out << format_path_values(entries);
  if (!out) {
    return std::unexpected(std::format("Kan inte skriva {}", file.string()));
  }
  return {};
}

} // namespace cratchit
