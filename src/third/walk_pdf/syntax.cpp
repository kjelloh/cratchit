#include "syntax.hpp"

#include "bytes.hpp"

namespace walk_pdf {

namespace {

// Deeper nesting than this is treated as damage, to protect the call stack
constexpr int max_nesting = 256;

// Past whitespace and comments
std::size_t skip_space(Bytes const& bytes, std::size_t pos) {
  while (true) {
    pos = bytes.skip_whitespace(pos);
    if (bytes.at(pos) != '%' || pos >= bytes.size()) {
      return pos;
    }
    pos = bytes.end_of_text(pos);
  }
}

std::size_t end_of_token(Bytes const& bytes, std::size_t pos) {
  while (pos < bytes.size() && is_regular(bytes.at(pos))) {
    ++pos;
  }
  return pos;
}

// (literal string): parentheses nest, and a backslash escapes the next byte
std::size_t end_of_literal_string(Bytes const& bytes, std::size_t pos) {
  int depth = 0;
  while (pos < bytes.size()) {
    auto const c = bytes.at(pos);
    if (c == '\\') {
      pos += 2;
      continue;
    }
    ++pos;
    if (c == '(') {
      ++depth;
    } else if (c == ')' && --depth == 0) {
      return pos;
    }
  }
  return bytes.size();
}

bool is_integer(std::string_view token) {
  auto digits = token;
  if (digits.starts_with('+') || digits.starts_with('-')) {
    digits.remove_prefix(1);
  }
  return !digits.empty() &&
         digits.find_first_not_of("0123456789") == std::string_view::npos;
}

bool is_unsigned(std::string_view token) {
  return !token.empty() && token.find_first_not_of("0123456789") == std::string_view::npos;
}

std::optional<Value> value_at(Bytes const& bytes, std::size_t pos, int nesting);

// The end of a dictionary or array whose children start at pos
std::size_t end_of_container(Bytes const& bytes, std::size_t pos, std::string_view close,
                             int nesting) {
  while (true) {
    pos = skip_space(bytes, pos);
    if (pos >= bytes.size()) {
      return bytes.size(); // Damaged: never closed
    }
    if (bytes.text(pos, std::min(pos + close.size(), bytes.size())) == close) {
      return pos + close.size();
    }
    auto const value = value_at(bytes, pos, nesting + 1);
    if (!value) {
      ++pos; // A stray closing byte, e.g. ']' in a dictionary: step over it
      continue;
    }
    pos = value->end;
  }
}

std::optional<Value> value_at(Bytes const& bytes, std::size_t pos, int nesting) {
  pos = skip_space(bytes, pos);
  if (pos >= bytes.size()) {
    return std::nullopt;
  }
  auto const c = bytes.at(pos);
  auto const next = bytes.at(pos + 1);

  if (nesting > max_nesting) {
    return Value{Kind::unknown, pos, bytes.size()};
  }
  if (c == '<' && next == '<') {
    return Value{Kind::dictionary, pos, end_of_container(bytes, pos + 2, ">>", nesting)};
  }
  if (c == '[') {
    return Value{Kind::array, pos, end_of_container(bytes, pos + 1, "]", nesting)};
  }
  if (c == ']' || (c == '>' && next == '>')) {
    return std::nullopt; // The end of the enclosing container
  }
  if (c == '<') {
    auto const close = bytes.text(pos, bytes.size()).find('>');
    return Value{Kind::string,
                 pos, close == std::string_view::npos ? bytes.size() : pos + close + 1};
  }
  if (c == '(') {
    return Value{Kind::string, pos, end_of_literal_string(bytes, pos)};
  }
  if (c == '/') {
    return Value{Kind::name, pos, end_of_token(bytes, pos + 1)};
  }
  if (!is_regular(c)) {
    return Value{Kind::unknown, pos, pos + 1}; // e.g. a stray ')' or '{'
  }

  auto const end = end_of_token(bytes, pos);
  auto const token = bytes.text(pos, end);
  if (is_integer(token)) {
    // "12 0 R" is a reference to object 12
    if (is_unsigned(token)) {
      auto const second = skip_space(bytes, end);
      auto const second_end = end_of_token(bytes, second);
      if (second_end > second && is_unsigned(bytes.text(second, second_end))) {
        auto const r = skip_space(bytes, second_end);
        if (bytes.at(r) == 'R' && !is_regular(bytes.at(r + 1))) {
          return Value{Kind::reference, pos, r + 1};
        }
      }
    }
    return Value{Kind::integer, pos, end};
  }
  if (token.find_first_not_of("+-.0123456789") == std::string_view::npos) {
    return Value{Kind::real, pos, end};
  }
  if (token == "true" || token == "false") {
    return Value{Kind::boolean, pos, end};
  }
  if (token == "null") {
    return Value{Kind::null, pos, end};
  }
  return Value{Kind::unknown, pos, end};
}

} // namespace

std::optional<Value> value_at(std::string_view bytes, std::size_t pos, std::size_t end) {
  return value_at(Bytes{bytes.substr(0, end)}, pos, 0);
}

std::optional<Operation> next_operation(std::string_view bytes, std::size_t pos, std::size_t end) {
  Bytes const limited{bytes.substr(0, end)};
  std::optional<std::size_t> begin;
  while (true) {
    auto const value = value_at(limited, pos, 0);
    if (!value) {
      // Only space left, or a stray "]" or ">>": step over it
      auto const at = skip_space(limited, pos);
      if (at >= limited.size()) {
        return std::nullopt;
      }
      pos = at + 1;
      continue;
    }
    if (value->kind != Kind::unknown || !is_regular(limited.at(value->begin))) {
      // An operand (or a stray delimiter, kept with the operands)
      begin = begin.value_or(value->begin);
      pos = value->end;
      continue;
    }
    auto const name = bytes.substr(value->begin, value->end - value->begin);
    Operation operation{begin.value_or(value->begin), value->begin, value->end, name};
    if (name == "BI") {
      // Inline image: its parameters, "ID", one byte of space, its data, "EI"
      operation.begin = value->begin;
      auto const id = limited.find_keyword(value->end, "ID");
      auto const data = id ? *id + 3 : limited.size();
      auto const ei = limited.find_keyword(std::min(data, limited.size()), "EI");
      operation.end = ei ? *ei + 2 : limited.size();
    }
    return operation;
  }
}

std::size_t first_child_at(Value const& container) {
  return container.begin + (container.kind == Kind::dictionary ? 2 : 1);
}

std::optional<Child> next_child(std::string_view bytes, Value const& container, std::size_t pos) {
  // The container's closing ">>" or "]" is not part of its children.
  // (A damaged container may have none, and run to the end of its object.)
  auto const close = container.kind == Kind::dictionary ? std::string_view{">>"} : "]";
  auto end = container.end;
  if (end >= first_child_at(container) + close.size() &&
      bytes.substr(end - close.size(), close.size()) == close) {
    end -= close.size();
  }
  Bytes const limited{bytes.substr(0, end)};

  if (container.kind == Kind::array) {
    auto const item = value_at(limited, pos, 1);
    if (!item) {
      return std::nullopt;
    }
    return Child{{}, *item};
  }
  if (container.kind != Kind::dictionary) {
    return std::nullopt;
  }
  // An entry is a name and a value. Skip anything else, as damage.
  while (true) {
    auto const key = value_at(limited, pos, 1);
    if (!key) {
      return std::nullopt;
    }
    if (key->kind != Kind::name) {
      pos = key->end;
      continue;
    }
    auto const value = value_at(limited, key->end, 1);
    if (!value) {
      return std::nullopt; // A key without a value at the end
    }
    return Child{decode_name(bytes.substr(key->begin, key->end - key->begin)), *value};
  }
}

std::optional<Value> stream_data(std::string_view bytes, std::size_t pos, std::size_t end) {
  Bytes const limited{bytes.substr(0, end)};
  auto const keyword = limited.skip_whitespace(pos);
  if (!limited.keyword_at(keyword, "stream")) {
    return std::nullopt;
  }
  // "stream" is followed by CR LF or LF (some writers use CR alone)
  auto begin = keyword + 6;
  if (limited.at(begin) == '\r') {
    ++begin;
  }
  if (limited.at(begin) == '\n') {
    ++begin;
  }
  auto data_end = limited.find_keyword(begin, "endstream").value_or(limited.size());
  // An EOL before "endstream" is not part of the data
  if (data_end > begin && limited.at(data_end - 1) == '\n') {
    --data_end;
  }
  if (data_end > begin && limited.at(data_end - 1) == '\r') {
    --data_end;
  }
  return Value{Kind::stream, std::min(begin, data_end), data_end};
}

std::string decode_name(std::string_view name) {
  if (name.starts_with('/')) {
    name.remove_prefix(1);
  }
  auto const hex = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
  };
  std::string result;
  for (std::size_t i = 0; i < name.size(); ++i) {
    if (name[i] == '#' && i + 2 < name.size() && hex(name[i + 1]) >= 0 &&
        hex(name[i + 2]) >= 0) {
      result += static_cast<char>(hex(name[i + 1]) * 16 + hex(name[i + 2]));
      i += 2;
    } else {
      result += name[i];
    }
  }
  return result;
}

} // namespace walk_pdf
