#include "pyroceph/tui/path_completion.hpp"

#include "charset/utf8.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <system_error>
#include <vector>

namespace pyroceph::tui {

namespace fs = std::filesystem;

namespace {

#ifdef _WIN32
constexpr std::u32string_view separators = U"\\/";
#else
constexpr std::u32string_view separators = U"/";
#endif

std::u32string decoded(std::u8string_view bytes) {
  charset::utf8::Decoder decoder;
  std::u32string result;
  for (auto const byte : bytes) {
    if (auto const code_point = decoder.push(static_cast<std::uint8_t>(byte))) result.push_back(*code_point);
  }
  return result;
}

std::u8string encoded(std::u32string_view text) {
  charset::utf8::Encoder encoder;
  std::u8string result;
  for (auto const code_point : text) {
    for (auto const byte : encoder.push(code_point)) result.push_back(static_cast<char8_t>(byte));
  }
  return result;
}

std::string utf8_of(std::u32string_view text) {
  auto const bytes = encoded(text);
  return std::string(bytes.begin(), bytes.end());
}

bool same(CodePoint a, CodePoint b) {
#ifdef _WIN32
  // Windows names ignore case
  auto lower = [](CodePoint c) { return c >= U'A' && c <= U'Z' ? c - U'A' + U'a' : c; };
  return lower(a) == lower(b);
#else
  return a == b;
#endif
}

bool starts_with(std::u32string_view text, std::u32string_view prefix) {
  return text.size() >= prefix.size() && std::ranges::equal(text.substr(0, prefix.size()), prefix, same);
}

} // namespace

std::u32string path_input_of(fs::path const& folder) {
  auto text = decoded(folder.u8string());
  if (text.empty() || separators.find(text.back()) == std::u32string_view::npos) {
    text.push_back(static_cast<CodePoint>(fs::path::preferred_separator));
  }
  return text;
}

fs::path path_of(std::u32string_view typed) { return fs::path(encoded(typed)); }

Completion complete_path(std::u32string_view typed, bool files,
                         std::function<bool(fs::path const&)> const& accept_file, DescribeFile const& describe_file,
                         ListCandidates const& list) {
  auto const cut = typed.find_last_of(separators);
  if (cut == std::u32string_view::npos) return {};
  auto const folder = path_of(typed.substr(0, cut + 1));
  auto const prefix = typed.substr(cut + 1);

  struct Name {
    std::u32string text;
    bool is_folder;
  };
  std::vector<Name> names;
  std::error_code ec;
  for (fs::directory_iterator it(folder, fs::directory_options::skip_permission_denied, ec), end; !ec && it != end;
       it.increment(ec)) {
    std::error_code type_ec;
    bool const is_folder = it->is_directory(type_ec);
    if (!is_folder && !(files && it->is_regular_file(type_ec) && (!accept_file || accept_file(it->path())))) continue;
    auto name = decoded(it->path().filename().u8string());
    if (name.starts_with(U'.') && !prefix.starts_with(U'.')) continue;
    if (starts_with(name, prefix)) names.push_back({std::move(name), is_folder});
  }
  if (names.empty()) return {};
  std::ranges::sort(names, {}, &Name::text);
  auto const describe = [&](Name const& name) -> std::vector<std::string> {
    if (name.is_folder || !describe_file) return {};
    return describe_file(folder / path_of(name.text));
  };
  if (names.size() == 1) {
    auto added = names.front().text.substr(prefix.size());
    if (names.front().is_folder) added.push_back(static_cast<CodePoint>(fs::path::preferred_separator));
    return {std::move(added), {}, describe(names.front())};
  }
  auto agreed = names.front().text.size();
  for (auto const& name : names) {
    auto const differs = std::ranges::mismatch(names.front().text, name.text, same).in1;
    agreed = std::min(agreed, static_cast<std::size_t>(differs - names.front().text.begin()));
  }
  if (agreed > prefix.size()) return {names.front().text.substr(prefix.size(), agreed - prefix.size()), {}, {}};
  Completion result;
  if (list) {
    std::vector<fs::path> found_files;
    std::vector<fs::path> found_folders;
    for (auto const& name : names) (name.is_folder ? found_folders : found_files).push_back(folder / path_of(name.text));
    result.candidates = list(found_files, found_folders);
    return result;
  }
  for (auto const& name : names) {
    result.candidates.push_back(utf8_of(name.text));
    for (auto const& line : describe(name)) result.candidates.push_back("    " + line);
  }
  return result;
}

} // namespace pyroceph::tui
