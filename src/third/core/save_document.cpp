#include "core/save_document.hpp"

#include <format>
#include <fstream>
#include <string_view>
#include <system_error>

namespace fs = std::filesystem;

namespace cratchit {

namespace {

// A path from UTF-8 text (fs::path(std::string) is not UTF-8 on Windows)
fs::path path_of(std::string const& text) {
  return fs::path(std::u8string(text.begin(), text.end()));
}

bool not_allowed(char c) {
  return static_cast<unsigned char>(c) < 0x20 || std::string_view("\\/:*?\"<>|").find(c) != std::string_view::npos;
}

} // namespace

std::string file_name_of(std::string_view title) {
  std::string name;
  for (auto const c : title) name.push_back(not_allowed(c) ? '-' : c);
  while (!name.empty() && (name.back() == ' ' || name.back() == '.')) name.pop_back();
  return name.empty() ? std::string("dokument") : name;
}

std::string utf8_of(fs::path const& path) {
  auto const text = path.u8string();
  return std::string(text.begin(), text.end());
}

std::expected<fs::path, std::string> save_document(fs::path const& folder, std::string_view title,
                                                   std::vector<std::string> const& lines) {
  auto const name = file_name_of(title);
  std::error_code ec;
  fs::path file;
  for (int n = 1;; ++n) {
    file = folder / path_of(n == 1 ? name + ".txt" : std::format("{} ({}).txt", name, n));
    if (!fs::exists(file, ec) && !ec) break;
    if (ec) return std::unexpected(std::format("Kan inte se om {} finns: {}", utf8_of(file), ec.message()));
  }
  std::ofstream out(file, std::ios::binary);
  if (!out) return std::unexpected(std::format("Kan inte skapa {}", utf8_of(file)));
  for (auto const& line : lines) out << line << '\n';
  out.close();
  if (!out) return std::unexpected(std::format("Kan inte skriva {}", utf8_of(file)));
  return file;
}

} // namespace cratchit
