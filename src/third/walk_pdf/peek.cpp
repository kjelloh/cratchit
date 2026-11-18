#include "peek.hpp"

#include "bytes.hpp"
#include "syntax.hpp"
#include "text_string.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <string_view>
#include <vector>

namespace walk_pdf {

namespace {

bool is_printable(char c) { return c >= ' ' && c <= '~'; }

// Text is printable ASCII and whitespace. Allow some other bytes, as PDF text
// may hold e.g. Latin-1 letters, but call it binary if more than a quarter are.
// A zero byte is PDF whitespace, but not text: zeros are binary data.
bool is_binary(std::string_view bytes) {
  auto const other = std::ranges::count_if(
      bytes, [](char c) { return c == '\0' || (!is_printable(c) && !is_whitespace(c)); });
  return static_cast<std::size_t>(other) * 4 > bytes.size();
}

// Whitespace runs as one space and other bytes outside printable ASCII as '.',
// trimmed. Stops a little after limit characters, as the rest is cut anyway.
std::string squeeze(std::string_view bytes, std::size_t limit) {
  std::string result;
  bool space = false;
  for (auto const c : bytes) {
    if (result.size() > limit) {
      break;
    }
    if (is_whitespace(c)) {
      space = !result.empty();
      continue;
    }
    if (space) {
      result += ' ';
      space = false;
    }
    result += is_printable(c) ? c : '.';
  }
  return result;
}

bool continues_a_character(char c) { return (static_cast<unsigned char>(c) & 0xC0) == 0x80; }

// Characters in UTF-8 text
std::size_t length(std::string_view text) {
  return static_cast<std::size_t>(std::ranges::count_if(text, [](char c) { return !continues_a_character(c); }));
}

// UTF-8 text cut to width characters, ending with "..." when cut. Cuts
// between characters, never inside one.
std::string fit(std::string text, std::size_t width) {
  if (length(text) <= width) {
    return text;
  }
  auto const keep = std::max<std::size_t>(width, 3) - 3;
  std::size_t characters = 0;
  std::size_t end = 0;
  for (; end < text.size(); ++end) {
    if (!continues_a_character(text[end])) {
      if (characters == keep) {
        break;
      }
      ++characters;
    }
  }
  text.resize(end);
  return text + "...";
}

// Text decoded from a string, on one line: control characters and runs of
// whitespace as one space (kept at the ends too, so " " shows as " ")
std::string display_text(std::string_view utf8) {
  std::string result;
  for (auto const c : utf8) {
    auto const byte = static_cast<unsigned char>(c);
    if (byte < 0x20 || byte == 0x7F || c == ' ') {
      if (result.empty() || result.back() != ' ') {
        result += ' ';
      }
      continue;
    }
    result += c;
  }
  return result;
}

// Does it start like PDF syntax: a dictionary, array, string or name?
// Then it is shown as text even if its strings hold binary (e.g. encrypted) bytes.
bool starts_as_syntax(std::string_view bytes) {
  Bytes const all{bytes};
  auto const c = all.at(all.skip_whitespace(0));
  return c == '<' || c == '[' || c == '(' || c == '/';
}

std::string text_or_binary(std::string_view bytes, std::size_t width) {
  if (!starts_as_syntax(bytes) && is_binary(bytes)) {
    return std::format("<{} bytes, binary>", bytes.size());
  }
  return fit(squeeze(bytes, width), width);
}

// A string token shown decoded in quotes when it is marked as UTF-16 or
// UTF-8 text. Otherwise as it is in the file, or, when it is encrypted, its
// decrypted bytes in parentheses. content is the element the token is in.
std::string show_string(Content const& content, std::string_view token, std::size_t limit) {
  auto const bytes = string_bytes(token);
  auto const decrypted = content.document().decrypt(content.path(), bytes);
  if (auto const text = text_to_utf8(decrypted.value_or(bytes))) {
    return "\"" + display_text(*text) + "\"";
  }
  return decrypted ? "(" + squeeze(*decrypted, limit) + ")" : squeeze(token, limit);
}

// A value on one line. Dictionaries and arrays are rebuilt from their
// children, so that strings in them are shown decoded. Stops soon after out
// is longer than limit bytes, as the rest is cut anyway.
void render(Content const& content, std::string_view bytes, Value const& value, std::size_t limit,
            std::string& out) {
  if (out.size() > limit) {
    return;
  }
  auto const token = bytes.substr(value.begin, value.end - value.begin);
  switch (value.kind) {
  case Kind::dictionary:
  case Kind::array: {
    auto const is_dictionary = value.kind == Kind::dictionary;
    out += is_dictionary ? "<<" : "[";
    bool first = true;
    for (auto pos = first_child_at(value); out.size() <= limit;) {
      auto const child = next_child(bytes, value, pos);
      if (!child) {
        break;
      }
      pos = child->value.end;
      if (is_dictionary) {
        out += " /" + squeeze(child->key, limit) + " ";
      } else if (!first) {
        out += ' ';
      }
      first = false;
      render(content, bytes, child->value, limit, out);
    }
    out += is_dictionary ? " >>" : "]";
    break;
  }
  case Kind::string: out += show_string(content, token, limit); break;
  default: out += squeeze(token, limit); break;
  }
}

// A value's peek; a token that is not PDF syntax may be binary
std::string peek_value(Content const& content, std::string_view bytes, Value const& value,
                       std::size_t width) {
  if (value.kind == Kind::unknown) {
    return text_or_binary(bytes.substr(value.begin, value.end - value.begin), width);
  }
  std::string out;
  render(content, bytes, value, 4 * width, out); // UTF-8 may take up to 4 bytes per character
  return fit(std::move(out), width);
}

std::string count(std::size_t n, std::string_view singular, std::string_view plural) {
  return std::format("{} {}", n, n == 1 ? singular : plural);
}

std::string join(std::vector<std::string> const& parts, std::string_view separator) {
  std::string result;
  for (auto const& part : parts) {
    if (!result.empty()) {
      result += separator;
    }
    result += part;
  }
  return result;
}

// The bytes after a leading keyword such as "trailer"
std::string_view after(std::string_view bytes, std::string_view keyword) {
  return bytes.starts_with(keyword) ? bytes.substr(keyword.size()) : bytes;
}

// The names in a dictionary's /Filter entry: a name, or an array of names
std::string filter_names(std::string_view bytes, Value const& dictionary) {
  for (auto pos = first_child_at(dictionary);;) {
    auto const entry = next_child(bytes, dictionary, pos);
    if (!entry) {
      return {};
    }
    pos = entry->value.end;
    if (entry->key != "Filter") {
      continue;
    }
    auto const& value = entry->value;
    if (value.kind == Kind::name) {
      return decode_name(bytes.substr(value.begin, value.end - value.begin));
    }
    std::vector<std::string> names;
    if (value.kind == Kind::array) {
      for (auto at = first_child_at(value);;) {
        auto const item = next_child(bytes, value, at);
        if (!item) {
          break;
        }
        at = item->value.end;
        if (item->value.kind == Kind::name) {
          names.push_back(
              decode_name(bytes.substr(item->value.begin, item->value.end - item->value.begin)));
        }
      }
    }
    return join(names, " ");
  }
}

std::string peek_header(std::string_view bytes) {
  return "version " + squeeze(after(bytes, "%PDF-"), 20);
}

std::string peek_comment(std::string_view bytes, std::size_t width) {
  auto const text = after(bytes, "%");
  // A comment of only non-ASCII bytes near the start says the file holds binary data
  if (!text.empty() && std::ranges::all_of(text, [](char c) { return (c & 0x80) != 0; })) {
    return std::format("<binary marker, {} bytes>", text.size());
  }
  return text_or_binary(text, width);
}

// An object's body, after "n g obj". If it holds a stream, its dictionary is
// shown and the stream data described.
std::string peek_object(Content const& content, std::size_t width) {
  auto const bytes = content.bytes();
  Bytes const all{bytes};
  // An object in an object stream (not in the file) is just its value, without "n g obj"
  auto const begin = content.in_file() ? all.find_keyword(0, "obj").value_or(0) + 3 : std::size_t{0};
  auto end = bytes.size();
  if (auto const endobj = bytes.rfind("endobj");
      endobj != std::string_view::npos && endobj >= begin && all.keyword_at(endobj, "endobj")) {
    end = endobj;
  }
  auto const body = value_at(bytes, begin, end);
  auto const stream = body && body->kind == Kind::dictionary
                          ? stream_data(bytes, body->end, end)
                          : std::nullopt;
  if (!stream) {
    return body ? peek_value(content, bytes, *body, width)
                : text_or_binary(bytes.substr(begin, end - begin), width);
  }
  auto const filters = filter_names(bytes, *body);
  auto const size = stream->end - stream->begin;
  auto const description = filters.empty() ? std::format(" + stream <{} bytes>", size)
                                           : std::format(" + stream <{} bytes, {}>", size, filters);
  // Always show the description, and the dictionary in whatever width is left
  auto const dictionary_width =
      std::max<std::size_t>(width > description.size() ? width - description.size() : 0, 20);
  return peek_value(content, bytes, *body, dictionary_width) + description;
}

// Entries and object numbers from the subsection lines "first count"
std::string peek_xref(std::string_view bytes) {
  Bytes const all{bytes};
  auto pos = all.end_of_line(0); // past "xref"
  std::size_t subsections = 0;
  std::uint64_t entries = 0;
  std::uint64_t first_object = 0;
  while (pos < all.size()) {
    pos = all.skip_whitespace(pos);
    auto const first = all.integer_at<std::uint64_t>(pos);
    if (!first) {
      break;
    }
    auto const n = all.integer_at<std::uint64_t>(all.skip_whitespace(first->second));
    if (!n) {
      break;
    }
    if (subsections == 0) {
      first_object = first->first;
    }
    ++subsections;
    entries += n->first;
    pos = all.end_of_line(n->second);
    for (std::uint64_t i = 0; i < n->first && pos < all.size(); ++i) {
      pos = all.end_of_line(pos); // one line per entry
    }
  }
  if (subsections == 1 && entries > 0) {
    return std::format("{}, objects {}-{}", count(entries, "entry", "entries"), first_object,
                       first_object + entries - 1);
  }
  return std::format("{} in {}", count(entries, "entry", "entries"),
                     count(subsections, "subsection", "subsections"));
}

// How many children of each kind
std::array<std::size_t, kind_count> count_children(Content const& content, std::size_t& total) {
  std::array<std::size_t, kind_count> counts{};
  total = 0;
  for (auto const& [path, child] : content) {
    ++counts[static_cast<std::size_t>(child.kind())];
    ++total;
  }
  return counts;
}

std::string peek_revision(Content const& content) {
  std::size_t total = 0;
  auto const counts = count_children(content, total);
  auto const of = [&](Kind kind) { return counts[static_cast<std::size_t>(kind)]; };

  std::vector<std::string> parts;
  parts.push_back(count(of(Kind::object), "object", "objects"));
  for (auto const kind : {Kind::xref, Kind::trailer}) {
    if (of(kind) == 1) {
      parts.emplace_back(to_string(kind));
    } else if (of(kind) > 1) {
      parts.push_back(std::format("{} {}", of(kind), to_string(kind)));
    }
  }
  if (of(Kind::unknown) > 0) {
    parts.push_back(std::format("{} unknown", of(Kind::unknown)));
  }
  return std::format("{}: {}", count(total, "element", "elements"), join(parts, ", "));
}

// The revisions, and how the file is encrypted
std::string peek_document(Content const& content) {
  std::size_t total = 0;
  count_children(content, total);
  auto result = count(total, "revision", "revisions");
  if (auto const pages = content.document().pages().size(); pages > 0) {
    result += "; " + count(pages, "page", "pages") + " (/pages)";
  }
  if (auto const& security = content.document().security()) {
    result += "; encrypted: " + security->status;
    if (auto const denied = security->denied(); !denied.empty()) {
      result += "; not permitted: " + join(denied, ", ");
    }
  }
  return result;
}

} // namespace

std::string peek(Content const& content, std::size_t width) {
  auto const bytes = content.bytes();
  switch (content.kind()) {
  case Kind::document: return peek_document(content);
  case Kind::revision: return peek_revision(content);
  case Kind::header: return peek_header(bytes);
  case Kind::comment: return peek_comment(bytes, width);
  case Kind::object: return peek_object(content, width);
  case Kind::xref: return peek_xref(bytes);
  case Kind::trailer: {
    auto const body = value_at(bytes, std::string_view{"trailer"}.size(), bytes.size());
    return body ? peek_value(content, bytes, *body, width) : text_or_binary(after(bytes, "trailer"), width);
  }
  case Kind::startxref: return squeeze(after(bytes, "startxref"), width);
  case Kind::eof: return {};
  case Kind::unknown: return text_or_binary(bytes, width);
  case Kind::reference: {
    // Where it leads
    auto const target = content.document().resolve(content.path());
    return squeeze(bytes, width) + " -> " + (target ? target->to_string() : std::string{"not found"});
  }
  case Kind::null:
  case Kind::boolean:
  case Kind::integer:
  case Kind::real:
  case Kind::name:
  case Kind::string:
  case Kind::array:
  case Kind::dictionary: {
    auto const value = value_at(bytes, 0, bytes.size());
    return value ? peek_value(content, bytes, *value, width) : std::string{};
  }
  case Kind::stream: {
    // Stream data that is text (e.g. an uncompressed page) is shown, other
    // data described with its filters. Only its start is needed, and decrypted.
    auto const start = bytes.substr(0, 1024);
    auto const data = content.document().decrypt(content.path(), start).value_or(std::string{start});
    if (!is_binary(data)) {
      return fit(squeeze(data, width), width);
    }
    std::vector<std::string> names;
    for (auto const& filter : content.document().filters(content.path())) {
      names.push_back(filter.name);
    }
    return names.empty() ? std::format("<{} bytes>", bytes.size())
                         : std::format("<{} bytes, {}>", bytes.size(), join(names, " "));
  }
  case Kind::decoded: {
    // As stream data, but already decrypted and decoded; says if decoding stopped early
    auto const start = bytes.substr(0, 1024);
    auto text = is_binary(start) ? std::format("<{} bytes>", bytes.size()) : fit(squeeze(start, width), width);
    if (auto const decoded = content.document().decode(content.path().parent()); decoded && decoded->problem) {
      text = "(" + *decoded->problem + ") " + text;
    }
    return text;
  }
  case Kind::text_block: {
    // The text it shows, through its fonts
    if (auto const text = content.document().text(content.path())) {
      return fit("\"" + display_text(*text) + "\"", width);
    }
    return text_or_binary(bytes, width);
  }
  case Kind::operation: {
    if (bytes.starts_with("BI")) {
      return std::format("<inline image, {} bytes>", bytes.size());
    }
    // A text-showing operator: its name and the text it shows, through its font
    if (auto const shown = content.document().text(content.path())) {
      auto const name = next_operation(bytes, 0, bytes.size());
      return fit(std::string{name ? name->name : ""} + " \"" + display_text(*shown) + "\"", width);
    }
    auto text = text_or_binary(bytes, width);
    // "/F8 16 Tf": which font /F8 is
    if (bytes.ends_with("Tf")) {
      if (auto const name = value_at(bytes, 0, bytes.size()); name && name->kind == Kind::name) {
        auto const font = content.document().font(content.path(), decode_name(bytes.substr(name->begin, name->end - name->begin)));
        text += " -> " + (font ? font->to_string() : std::string{"font not found"});
      }
    }
    return text;
  }
  case Kind::pages: {
    auto const count = content.document().pages().size();
    return std::format("{} {}, in page tree order", count, count == 1 ? "page" : "pages");
  }
  case Kind::page: {
    auto const& pages = content.document().pages();
    auto const index = content.path().segments().back().number;
    return index < pages.size() ? "-> " + pages[index].to_string() : std::string{};
  }
  case Kind::key:
  case Kind::index: return {}; // Only used in path segments
  }
  return {};
}

} // namespace walk_pdf
