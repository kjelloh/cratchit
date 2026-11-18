#include "core/manual.hpp"

#include <format>
#include <fstream>
#include <optional>
#include <sstream>

namespace cratchit {

namespace {

constexpr std::string_view comment_start = "<!--";
constexpr std::string_view comment_end = "-->";
constexpr std::string_view help_word = "hjälp:";

std::string_view trimmed(std::string_view text) {
  auto const first = text.find_first_not_of(" \t\r");
  if (first == std::string_view::npos) return {};
  auto const last = text.find_last_not_of(" \t\r");
  return text.substr(first, last - first + 1);
}

// The topic of a marker line, or nothing when the line is no marker
std::optional<std::string_view> topic_of(std::string_view line) {
  line = trimmed(line);
  if (!line.starts_with(comment_start) || !line.ends_with(comment_end)) return std::nullopt;
  line.remove_prefix(comment_start.size());
  line.remove_suffix(comment_end.size());
  line = trimmed(line);
  if (!line.starts_with(help_word)) return std::nullopt;
  line.remove_prefix(help_word.size());
  return trimmed(line);
}

std::vector<std::string_view> lines_of(std::string_view text) {
  std::vector<std::string_view> lines;
  while (!text.empty()) {
    auto const end = text.find('\n');
    lines.push_back(text.substr(0, end));
    if (end == std::string_view::npos) break;
    text.remove_prefix(end + 1);
  }
  return lines;
}

} // namespace

std::string plain_text_of(std::string_view markdown) {
  std::string result;
  if (markdown.starts_with("* ")) {
    result = "- ";
    markdown.remove_prefix(2);
  }
  for (std::size_t i = 0; i < markdown.size(); ++i) {
    auto const c = markdown[i];
    if (c == '*' || c == '`') continue;
    if (c == '[') {
      // [text](target) shows as text
      auto const close = markdown.find("](", i);
      auto const end = close == std::string_view::npos ? close : markdown.find(')', close);
      if (end != std::string_view::npos) {
        result += plain_text_of(markdown.substr(i + 1, close - i - 1));
        i = end;
        continue;
      }
    }
    result.push_back(c);
  }
  return result;
}

std::vector<std::string> Manual::help(std::string_view topic) const {
  std::vector<std::string> result;
  bool in_help = false;
  bool in_code = false;
  for (auto line : lines_of(text_)) {
    if (!in_help) {
      in_help = topic_of(line) == topic;
      continue;
    }
    auto const text = trimmed(line);
    if (text.starts_with('#') || text.starts_with(comment_start)) break;
    if (text.starts_with("```")) {
      in_code = !in_code;
      continue;
    }
    if (in_code) {
      // The lines of a code block show as they are
      if (line.ends_with('\r')) line.remove_suffix(1);
      result.emplace_back(line);
      continue;
    }
    if (text.empty() && result.empty()) continue; // blank lines before the text
    result.push_back(plain_text_of(text));
  }
  while (!result.empty() && result.back().empty()) result.pop_back();
  return result;
}

std::vector<std::string> Manual::topics() const {
  std::vector<std::string> result;
  for (auto const line : lines_of(text_)) {
    if (auto const topic = topic_of(line)) result.emplace_back(*topic);
  }
  return result;
}

std::expected<Manual, std::string> read_manual(std::filesystem::path const& file) {
  std::ifstream in(file, std::ios::binary);
  if (!in) return std::unexpected(std::format("{} går inte att läsa", file.string()));
  std::ostringstream text;
  text << in.rdbuf();
  return Manual(text.str());
}

} // namespace cratchit
