#include "path.hpp"

#include <array>
#include <charconv>
#include <format>
#include <optional>
#include <utility>

namespace walk_pdf {

std::string_view to_string(Kind kind) {
  switch (kind) {
  case Kind::document: return "document";
  case Kind::revision: return "revision";
  case Kind::header: return "header";
  case Kind::comment: return "comment";
  case Kind::object: return "object";
  case Kind::xref: return "xref";
  case Kind::trailer: return "trailer";
  case Kind::startxref: return "startxref";
  case Kind::eof: return "eof";
  case Kind::unknown: return "unknown";
  case Kind::null: return "null";
  case Kind::boolean: return "boolean";
  case Kind::integer: return "integer";
  case Kind::real: return "real";
  case Kind::name: return "name";
  case Kind::string: return "string";
  case Kind::array: return "array";
  case Kind::dictionary: return "dictionary";
  case Kind::reference: return "reference";
  case Kind::stream: return "stream";
  case Kind::decoded: return "decoded";
  case Kind::text_block: return "text block";
  case Kind::operation: return "operation";
  case Kind::pages: return "pages";
  case Kind::page: return "page";
  case Kind::key: return "key";
  case Kind::index: return "index";
  }
  return "?";
}

namespace {

// How each kind is named in a path segment. The document (root) has no segment.
constexpr std::array<std::pair<Kind, std::string_view>, 12> segment_names{{
    {Kind::revision, "rev"},
    {Kind::header, "header"},
    {Kind::comment, "comment"},
    {Kind::object, "obj"},
    {Kind::xref, "xref"},
    {Kind::trailer, "trailer"},
    {Kind::startxref, "startxref"},
    {Kind::eof, "eof"},
    {Kind::unknown, "unknown"},
    {Kind::text_block, "BT"},
    {Kind::operation, "op"},
    {Kind::page, "page"},
}};

std::string_view segment_name(Kind kind) {
  for (auto const& [k, name] : segment_names) {
    if (k == kind) {
      return name;
    }
  }
  return "?";
}

std::optional<Kind> segment_kind(std::string_view name) {
  for (auto const& [kind, n] : segment_names) {
    if (n == name) {
      return kind;
    }
  }
  return std::nullopt;
}

// An unsigned decimal number that is all of text
template <typename T> std::optional<T> parse_number(std::string_view text) {
  T value{};
  auto const* last = text.data() + text.size();
  auto const [ptr, ec] = std::from_chars(text.data(), last, value);
  if (text.empty() || ec != std::errc{} || ptr != last) {
    return std::nullopt;
  }
  return value;
}

// Bytes a key can be written with as they are: printable ASCII that is not
// whitespace, a PDF delimiter (which includes '/', '[' and ']'), or '#'
bool is_plain_key_byte(char c) {
  return c > ' ' && c <= '~' && c != '#' && c != '(' && c != ')' && c != '<' && c != '>' &&
         c != '[' && c != ']' && c != '{' && c != '}' && c != '/' && c != '%';
}

bool is_reserved_word(std::string_view text) {
  return text == "header" || text == "stream" || text == "decoded" || text == "pages";
}

std::string escape_key(std::string_view key) {
  if (key.empty()) {
    return "#"; // PDF allows the empty name "/"
  }
  std::string result;
  for (auto const c : key) {
    if (is_plain_key_byte(c)) {
      result += c;
    } else {
      result += std::format("#{:02X}", static_cast<unsigned char>(c));
    }
  }
  if (is_reserved_word(result)) {
    result = std::format("#{:02X}", static_cast<unsigned char>(result[0])) + result.substr(1);
  }
  return result;
}

std::optional<int> hex_digit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return std::nullopt;
}

std::expected<std::string, std::string> unescape_key(std::string_view text) {
  if (text == "#") {
    return std::string{};
  }
  std::string key;
  for (std::size_t i = 0; i < text.size(); ++i) {
    auto const c = text[i];
    if (c == '#') {
      auto const high = i + 1 < text.size() ? hex_digit(text[i + 1]) : std::nullopt;
      auto const low = i + 2 < text.size() ? hex_digit(text[i + 2]) : std::nullopt;
      if (!high || !low) {
        return std::unexpected(std::format("'#' must be followed by two hex digits in '{}'", text));
      }
      key += static_cast<char>(*high * 16 + *low);
      i += 2;
    } else if (is_plain_key_byte(c)) {
      key += c;
    } else {
      return std::unexpected(std::format("'{}' must be written #{:02X} in '{}'", c,
                                         static_cast<unsigned char>(c), text));
    }
  }
  return key;
}

std::expected<Segment, std::string> parse_segment(std::string_view text) {
  if (text.empty()) {
    return std::unexpected(std::string{"empty segment"});
  }
  if (text == "stream") {
    return Segment::stream();
  }
  if (text == "decoded") {
    return Segment::decoded();
  }
  if (text == "pages") {
    return Segment::pages();
  }
  if (text.starts_with('[')) {
    auto const index = text.ends_with(']') && text.size() > 2
                           ? parse_number<std::uint64_t>(text.substr(1, text.size() - 2))
                           : std::nullopt;
    if (!index) {
      return std::unexpected(std::format("expected '[index]' but got '{}'", text));
    }
    return Segment::item(*index);
  }
  auto const open = text.find('[');
  if (open == std::string_view::npos && text != "header") {
    auto key = unescape_key(text);
    if (!key) {
      return std::unexpected(key.error());
    }
    return Segment::entry(std::move(*key));
  }
  auto const name = text.substr(0, open);
  auto const kind = segment_kind(name);
  if (!kind) {
    return std::unexpected(std::format("unknown segment '{}'", text));
  }
  if (*kind == Kind::header) {
    if (open != std::string_view::npos) {
      return std::unexpected(std::format("'header' takes no [...] in '{}'", text));
    }
    return Segment::header();
  }
  if (open == std::string_view::npos || !text.ends_with(']')) {
    return std::unexpected(std::format("expected '{}[...]' but got '{}'", name, text));
  }
  auto const inside = text.substr(open + 1, text.size() - open - 2);
  auto const ordinal = parse_number<std::uint64_t>(inside);
  if (!ordinal) {
    return std::unexpected(std::format("expected '{}[number]' but got '{}'", name, text));
  }
  return Segment::ordinal(*kind, *ordinal);
}

} // namespace

std::string Segment::to_string() const {
  switch (kind) {
  case Kind::header: return "header";
  case Kind::stream: return "stream";
  case Kind::decoded: return "decoded";
  case Kind::pages: return "pages";
  case Kind::key: return escape_key(key);
  case Kind::index: return std::format("[{}]", number);
  default: return std::format("{}[{}]", segment_name(kind), number);
  }
}

Path Path::child(Segment segment) const {
  auto segments = segments_;
  segments.push_back(segment);
  return Path{std::move(segments)};
}

Path Path::parent() const {
  auto segments = segments_;
  if (!segments.empty()) {
    segments.pop_back();
  }
  return Path{std::move(segments)};
}

std::string Path::to_string() const {
  if (segments_.empty()) {
    return "/";
  }
  std::string result;
  for (auto const& segment : segments_) {
    result += '/';
    result += segment.to_string();
  }
  return result;
}

std::expected<Path, std::string> Path::parse(std::string_view text) {
  if (!text.starts_with('/')) {
    return std::unexpected(std::format("a path starts with '/' but got '{}'", text));
  }
  std::vector<Segment> segments;
  auto rest = text.substr(1);
  while (!rest.empty()) {
    auto const slash = rest.find('/');
    auto const part = rest.substr(0, slash);
    auto segment = parse_segment(part);
    if (!segment) {
      return std::unexpected(segment.error());
    }
    segments.push_back(*segment);
    if (slash == std::string_view::npos) {
      break;
    }
    rest = rest.substr(slash + 1);
    if (rest.empty()) {
      return std::unexpected(std::format("a path does not end with '/' but got '{}'", text));
    }
  }
  return Path{std::move(segments)};
}

} // namespace walk_pdf
