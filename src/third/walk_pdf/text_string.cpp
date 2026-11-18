#include "text_string.hpp"

#include "charset/utf16.hpp"
#include "charset/utf8.hpp"

namespace walk_pdf {

namespace {

std::optional<int> hex_value(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return std::nullopt;
}

bool is_octal(char c) { return c >= '0' && c <= '7'; }

// "<48 65 6C>" is "Hel". Whitespace is ignored, and an odd last digit is
// followed by 0.
std::string hex_string_bytes(std::string_view digits) {
  std::string result;
  std::optional<int> high;
  for (auto const c : digits) {
    auto const value = hex_value(c);
    if (!value) {
      continue; // whitespace (or damage)
    }
    if (high) {
      result += static_cast<char>(*high * 16 + *value);
      high.reset();
    } else {
      high = value;
    }
  }
  if (high) {
    result += static_cast<char>(*high * 16);
  }
  return result;
}

std::string literal_string_bytes(std::string_view text) {
  std::string result;
  for (std::size_t i = 0; i < text.size(); ++i) {
    auto const c = text[i];
    if (c == '\r') {
      // An end of line in the string is one LF, whatever it was in the file
      result += '\n';
      if (i + 1 < text.size() && text[i + 1] == '\n') {
        ++i;
      }
      continue;
    }
    if (c != '\\' || i + 1 == text.size()) {
      result += c;
      continue;
    }
    auto const e = text[++i];
    switch (e) {
    case 'n': result += '\n'; break;
    case 'r': result += '\r'; break;
    case 't': result += '\t'; break;
    case 'b': result += '\b'; break;
    case 'f': result += '\f'; break;
    case '\r': // A backslash before an end of line joins the lines
      if (i + 1 < text.size() && text[i + 1] == '\n') {
        ++i;
      }
      break;
    case '\n': break;
    default:
      if (is_octal(e)) {
        // \d, \dd or \ddd; a value above 255 keeps its low byte
        int value = e - '0';
        for (int digits = 1; digits < 3 && i + 1 < text.size() && is_octal(text[i + 1]);
             ++digits) {
          value = value * 8 + (text[++i] - '0');
        }
        result += static_cast<char>(value & 0xFF);
      } else {
        result += e; // \( \) \\ and any other: the byte itself
      }
    }
  }
  return result;
}

void append_utf8(std::string& text, charset::unicode::CodePoint code_point) {
  for (auto const byte : charset::utf8::Encoder{}.push(code_point)) {
    text += static_cast<char>(byte);
  }
}

} // namespace

std::string string_bytes(std::string_view token) {
  if (token.starts_with('(')) {
    token.remove_prefix(1);
    if (token.ends_with(')')) {
      token.remove_suffix(1);
    }
    return literal_string_bytes(token);
  }
  if (token.starts_with('<')) {
    token.remove_prefix(1);
    if (token.ends_with('>')) {
      token.remove_suffix(1);
    }
    return hex_string_bytes(token);
  }
  return std::string{token};
}

std::optional<std::string> text_to_utf8(std::string_view bytes) {
  std::string text;
  if (bytes.starts_with("\xFE\xFF")) {
    charset::utf16::Decoder decoder{charset::utf16::ByteOrder::big_endian};
    for (auto const byte : bytes.substr(2)) {
      if (auto const code_point = decoder.push(static_cast<charset::utf16::Byte>(byte))) {
        append_utf8(text, *code_point);
      }
    }
    if (auto const code_point = decoder.finish()) {
      append_utf8(text, *code_point);
    }
    return text;
  }
  if (bytes.starts_with("\xEF\xBB\xBF")) {
    // Decode and encode again, so that malformed bytes become U+FFFD
    charset::utf8::Decoder decoder;
    for (auto const byte : bytes.substr(3)) {
      if (auto const code_point = decoder.push(static_cast<charset::utf8::CodePoint>(byte))) {
        append_utf8(text, *code_point);
      }
    }
    if (auto const code_point = decoder.finish()) {
      append_utf8(text, *code_point);
    }
    return text;
  }
  return std::nullopt;
}

} // namespace walk_pdf
