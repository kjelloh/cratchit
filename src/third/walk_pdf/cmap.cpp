#include "cmap.hpp"

#include "charset/utf16.hpp"
#include "charset/utf8.hpp"
#include "glyph_names.hpp"
#include "syntax.hpp"
#include "text_string.hpp"

#include <algorithm>
#include <cstdint>

namespace walk_pdf {

namespace {

// A code's bytes as a number, for codes of up to four bytes
std::uint32_t number_of(std::string_view code) {
  std::uint32_t value = 0;
  for (auto const c : code) {
    value = (value << 8) | static_cast<unsigned char>(c);
  }
  return value;
}

std::string utf16_of(charset::unicode::CodePoint code_point) {
  std::string result;
  for (auto const byte : charset::utf16::Encoder{}.push(code_point)) {
    result += static_cast<char>(byte);
  }
  return result;
}

// A mapping's target: a hex string of UTF-16BE, or (in bfchar) a glyph name
std::string text_of(std::string_view data, Value const& value) {
  auto const token = data.substr(value.begin, value.end - value.begin);
  if (value.kind == Kind::name) {
    auto const code_point = glyph_unicode(decode_name(token));
    return utf16_of(code_point.value_or(charset::unicode::replacement_character));
  }
  return string_bytes(token);
}

} // namespace

CMap CMap::parse(std::string_view data) {
  CMap cmap;
  // The values from pos on, up to the keyword that ends the section
  auto const section = [&](std::size_t pos, std::string_view end_keyword) {
    std::vector<Value> values;
    while (auto const value = value_at(data, pos, data.size())) {
      pos = value->end;
      if (value->kind == Kind::unknown && data.substr(value->begin, value->end - value->begin) == end_keyword) {
        break;
      }
      values.push_back(*value);
    }
    return std::pair{values, pos};
  };
  auto const bytes_of = [&](Value const& value) {
    return string_bytes(data.substr(value.begin, value.end - value.begin));
  };

  std::size_t pos = 0;
  while (auto const value = value_at(data, pos, data.size())) {
    pos = value->end;
    if (value->kind != Kind::unknown) {
      continue;
    }
    auto const keyword = data.substr(value->begin, value->end - value->begin);
    if (keyword == "begincodespacerange") {
      auto const [values, next] = section(pos, "endcodespacerange");
      for (std::size_t i = 0; i + 1 < values.size(); i += 2) {
        auto low = bytes_of(values[i]);
        auto high = bytes_of(values[i + 1]);
        if (low.size() == high.size() && !low.empty()) {
          cmap.code_space_.push_back({std::move(low), std::move(high)});
        }
      }
      pos = next;
    } else if (keyword == "beginbfchar") {
      auto const [values, next] = section(pos, "endbfchar");
      for (std::size_t i = 0; i + 1 < values.size(); i += 2) {
        cmap.chars_.insert_or_assign(bytes_of(values[i]), text_of(data, values[i + 1]));
      }
      pos = next;
    } else if (keyword == "beginbfrange") {
      auto const [values, next] = section(pos, "endbfrange");
      for (std::size_t i = 0; i + 2 < values.size(); i += 3) {
        auto low = bytes_of(values[i]);
        auto high = bytes_of(values[i + 1]);
        if (low.size() != high.size() || low.empty() || low.size() > 4 || number_of(low) > number_of(high)) {
          continue;
        }
        auto const& target = values[i + 2];
        if (target.kind == Kind::array) {
          // One target per code, in order
          auto code = number_of(low);
          for (auto at = first_child_at(target); code <= number_of(high);) {
            auto const item = next_child(data, target, at);
            if (!item) {
              break;
            }
            at = item->value.end;
            std::string key = low;
            for (std::size_t b = 0; b < key.size(); ++b) {
              key[key.size() - 1 - b] = static_cast<char>((code >> (8 * b)) & 0xFF);
            }
            cmap.chars_.insert_or_assign(std::move(key), text_of(data, item->value));
            ++code;
          }
        } else {
          cmap.ranges_.push_back({std::move(low), std::move(high), text_of(data, target)});
        }
      }
      pos = next;
    }
  }
  return cmap;
}

std::optional<std::size_t> CMap::code_length(std::string_view bytes) const {
  for (std::size_t length = 1; length <= 4 && length <= bytes.size(); ++length) {
    for (auto const& range : code_space_) {
      if (range.low.size() != length) {
        continue;
      }
      bool inside = true;
      for (std::size_t i = 0; i < length && inside; ++i) {
        auto const b = static_cast<unsigned char>(bytes[i]);
        inside = b >= static_cast<unsigned char>(range.low[i]) && b <= static_cast<unsigned char>(range.high[i]);
      }
      if (inside) {
        return length;
      }
    }
  }
  return std::nullopt;
}

std::optional<std::string> CMap::utf8(std::string_view code) const {
  std::string utf16;
  if (auto const found = chars_.find(std::string{code}); found != chars_.end()) {
    utf16 = found->second;
  } else {
    auto const value = code.size() <= 4 ? number_of(code) : 0;
    auto const range = std::ranges::find_if(ranges_, [&](Range const& r) {
      return r.low.size() == code.size() && number_of(r.low) <= value && value <= number_of(r.high);
    });
    if (range == ranges_.end() || code.size() > 4) {
      return std::nullopt;
    }
    // Add the code's distance from low to the last code unit of the text
    utf16 = range->text;
    if (utf16.size() >= 2) {
      auto unit = (static_cast<std::uint32_t>(static_cast<unsigned char>(utf16[utf16.size() - 2])) << 8) |
                  static_cast<unsigned char>(utf16.back());
      unit += value - number_of(range->low);
      utf16[utf16.size() - 2] = static_cast<char>((unit >> 8) & 0xFF);
      utf16.back() = static_cast<char>(unit & 0xFF);
    }
  }
  // UTF-16BE to UTF-8, through the charset pipe stages
  std::string result;
  charset::utf16::Decoder decoder;
  charset::utf8::Encoder encoder;
  auto const put = [&](charset::unicode::CodePoint code_point) {
    for (auto const byte : encoder.push(code_point)) {
      result += static_cast<char>(byte);
    }
  };
  for (auto const byte : utf16) {
    if (auto const code_point = decoder.push(static_cast<charset::utf16::Byte>(byte))) {
      put(*code_point);
    }
  }
  if (auto const code_point = decoder.finish()) {
    put(*code_point);
  }
  return result;
}

} // namespace walk_pdf
