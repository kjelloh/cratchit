#pragma once

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <optional>
#include <string_view>
#include <system_error>
#include <utility>

namespace walk_pdf {

// PDF 32000-1:2008, 7.2.2 Character set
inline bool is_whitespace(char c) {
  return c == '\0' || c == '\t' || c == '\n' || c == '\f' || c == '\r' || c == ' ';
}

inline bool is_delimiter(char c) {
  return c == '(' || c == ')' || c == '<' || c == '>' || c == '[' || c == ']' || c == '{' ||
         c == '}' || c == '/' || c == '%';
}

inline bool is_regular(char c) { return !is_whitespace(c) && !is_delimiter(c); }

inline bool is_digit(char c) { return c >= '0' && c <= '9'; }

// Byte-level helpers over the bytes before an end offset
class Bytes {
public:
  explicit Bytes(std::string_view bytes) : bytes_(bytes) {}

  std::size_t size() const { return bytes_.size(); }
  char at(std::size_t pos) const { return pos < bytes_.size() ? bytes_[pos] : '\0'; }

  std::size_t skip_whitespace(std::size_t pos) const {
    while (pos < bytes_.size() && is_whitespace(bytes_[pos])) {
      ++pos;
    }
    return pos;
  }

  // Position of the line's end, before its EOL marker
  std::size_t end_of_text(std::size_t pos) const {
    while (pos < bytes_.size() && bytes_[pos] != '\n' && bytes_[pos] != '\r') {
      ++pos;
    }
    return pos;
  }

  // Position just after the line's EOL marker (CR, LF or CR LF)
  std::size_t end_of_line(std::size_t pos) const {
    pos = end_of_text(pos);
    if (pos < bytes_.size() && bytes_[pos] == '\r') {
      ++pos;
    }
    if (pos < bytes_.size() && bytes_[pos] == '\n') {
      ++pos;
    }
    return pos;
  }

  std::string_view text(std::size_t from, std::size_t to) const {
    return bytes_.substr(from, to - from);
  }

  // Is the keyword at pos and delimited on both sides?
  bool keyword_at(std::size_t pos, std::string_view keyword) const {
    if (bytes_.substr(pos, keyword.size()) != keyword) {
      return false;
    }
    if (pos > 0 && is_regular(bytes_[pos - 1])) {
      return false;
    }
    auto const after = pos + keyword.size();
    return after >= bytes_.size() || !is_regular(bytes_[after]);
  }

  // Position of the next delimited keyword at or after pos, and before limit
  std::optional<std::size_t> find_keyword(std::size_t pos, std::string_view keyword,
                                          std::size_t limit = std::string_view::npos) const {
    // Search only the bytes before limit; searching on past it costs time for nothing
    auto const searched = bytes_.substr(0, std::min(limit, bytes_.size()));
    while (true) {
      auto const found = searched.find(keyword, pos);
      if (found == std::string_view::npos) {
        return std::nullopt;
      }
      if (keyword_at(found, keyword)) {
        return found;
      }
      pos = found + 1;
    }
  }

  // An unsigned integer at pos, and the position after it
  template <typename T> std::optional<std::pair<T, std::size_t>> integer_at(std::size_t pos) const {
    if (pos >= bytes_.size()) {
      return std::nullopt;
    }
    T value{};
    auto const* first = bytes_.data() + pos;
    auto const* last = bytes_.data() + bytes_.size();
    auto const [ptr, ec] = std::from_chars(first, last, value);
    if (ec != std::errc{} || ptr == first || (ptr != last && is_regular(*ptr))) {
      return std::nullopt;
    }
    return std::pair{value, pos + static_cast<std::size_t>(ptr - first)};
  }

private:
  std::string_view bytes_;
};

} // namespace walk_pdf
