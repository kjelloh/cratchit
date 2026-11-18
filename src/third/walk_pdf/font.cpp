#include "font.hpp"

#include "charset/adobe_standard.hpp"
#include "charset/macroman.hpp"
#include "charset/utf8.hpp"
#include "charset/winansi.hpp"
#include "document.hpp"
#include "glyph_names.hpp"
#include "syntax.hpp"

#include <charconv>
#include <cstdlib>

namespace walk_pdf {

namespace {

std::optional<std::string> name_at(Document const& document, Path const& path) {
  auto const content = document.find(path);
  if (!content || content->kind() != Kind::name) {
    return std::nullopt;
  }
  return decode_name(content->bytes());
}

// A stream object's decoded data: the object a path leads to, then its stream
std::optional<std::string> stream_at(Document const& document, Path const& path) {
  auto const object = document.follow(path);
  if (!object) {
    return std::nullopt;
  }
  auto const decoded = document.decode(object->child(Segment::stream()));
  if (!decoded) {
    return std::nullopt;
  }
  return *decoded->bytes;
}

// A base encoding's code -> code point
std::optional<std::array<charset::unicode::CodePoint, 256>> base_encoding(std::string_view name) {
  std::array<charset::unicode::CodePoint, 256> table{};
  auto const fill = [&](auto decoder) {
    for (std::size_t code = 0; code < table.size(); ++code) {
      table[code] = decoder.push(static_cast<std::uint8_t>(code)).value_or(charset::unicode::replacement_character);
    }
    return table;
  };
  if (name == "WinAnsiEncoding") {
    return fill(charset::winansi::Decoder{});
  }
  if (name == "MacRomanEncoding") {
    return fill(charset::macroman::Decoder{});
  }
  if (name == "StandardEncoding") {
    return fill(charset::adobe_standard::Decoder{});
  }
  return std::nullopt; // e.g. MacExpertEncoding: not supported
}

// A number (integer or real) at path, if there is one there
std::optional<double> number_at(Document const& document, Path const& path) {
  auto const content = document.find(path);
  if (!content || (content->kind() != Kind::integer && content->kind() != Kind::real)) {
    return std::nullopt;
  }
  return std::strtod(std::string{content->bytes()}.c_str(), nullptr); // no from_chars for double in libc++
}

// The numbers of an array at path (or that a reference there leads to)
std::vector<std::optional<double>> numbers_at(Document const& document, Path const& path) {
  std::vector<std::optional<double>> result;
  if (auto const array = document.follow(path)) {
    for (auto const& item : document.children(*array)) {
      result.push_back(number_at(document, item.path));
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

Font Font::load(Document const& document, Path const& font) {
  Font result;
  auto const entry = [&](std::string key) { return font.child(Segment::entry(std::move(key))); };
  auto const subtype = name_at(document, entry("Subtype")).value_or("");

  if (auto const data = stream_at(document, entry("ToUnicode"))) {
    result.to_unicode_ = CMap::parse(*data);
  }

  if (subtype == "Type0") {
    // Composite font: codes by its CMap. Identity-H/V and most predefined
    // CMaps use two bytes; an embedded CMap says itself.
    result.code_bytes_ = 2;
    // Widths from the descendant CIDFont: /DW, and /W as "c [w1 w2 ...]" or "first last w"
    result.default_width_ = 1000;
    auto const descendants = document.follow(entry("DescendantFonts"));
    auto const cid_font = descendants ? document.follow(descendants->child(Segment::item(0))) : std::nullopt;
    if (cid_font) {
      result.default_width_ = number_at(document, cid_font->child(Segment::entry("DW"))).value_or(1000);
      result.has_widths_ = true;
      if (auto const w = document.follow(cid_font->child(Segment::entry("W")))) {
        std::vector<Content> items;
        for (auto const& item : document.children(*w)) {
          items.push_back(item.content);
        }
        auto const value = [&](std::size_t i) {
          return std::strtod(std::string{items[i].bytes()}.c_str(), nullptr);
        };
        for (std::size_t i = 0; i + 1 < items.size();) {
          auto const first = static_cast<std::uint32_t>(value(i));
          if (items[i + 1].kind() == Kind::array) {
            std::uint32_t cid = first;
            for (auto const& width : document.children(items[i + 1].path())) {
              result.cid_widths_[cid++] = std::strtod(std::string{width.content.bytes()}.c_str(), nullptr);
            }
            i += 2;
          } else if (i + 2 < items.size()) {
            auto const last = static_cast<std::uint32_t>(value(i + 1));
            for (auto cid = first; cid <= last && cid - first < 65536; ++cid) {
              result.cid_widths_[cid] = value(i + 2);
            }
            i += 3;
          } else {
            break;
          }
        }
      }
    }
    if (auto const data = stream_at(document, entry("Encoding"))) {
      if (auto cmap = CMap::parse(*data); cmap.has_code_space()) {
        result.encoding_ = std::move(cmap);
      }
    }
    return result;
  }

  // Simple font widths: /Widths from /FirstChar, else /MissingWidth. A Type 3
  // font's widths are in its glyph space, which /FontMatrix scales to text space.
  auto const first_char = number_at(document, entry("FirstChar")).value_or(0);
  auto const widths = numbers_at(document, entry("Widths"));
  auto const descriptor = document.follow(entry("FontDescriptor"));
  auto const missing = descriptor ? number_at(document, descriptor->child(Segment::entry("MissingWidth"))) : std::nullopt;
  auto scale = 1.0;
  if (subtype == "Type3") {
    auto const matrix = numbers_at(document, entry("FontMatrix"));
    scale = !matrix.empty() && matrix[0] ? *matrix[0] * 1000 : 1.0;
  }
  result.has_widths_ = !widths.empty();
  result.default_width_ = missing.value_or(widths.empty() ? 500.0 : 0.0) * (missing ? scale : 1.0);
  result.simple_widths_.fill(result.default_width_);
  for (std::size_t i = 0; i < widths.size(); ++i) {
    auto const code = static_cast<long long>(first_char) + static_cast<long long>(i);
    if (code >= 0 && code < 256 && widths[i]) {
      result.simple_widths_[static_cast<std::size_t>(code)] = *widths[i] * scale;
    }
  }

  // Simple font: one byte per code, mapped by a base encoding and /Differences
  auto base = subtype == "TrueType" ? std::string{"WinAnsiEncoding"} : std::string{"StandardEncoding"};
  std::optional<Path> differences;
  if (auto const name = name_at(document, entry("Encoding"))) {
    base = *name;
  } else if (auto const encoding = document.follow(entry("Encoding"))) {
    if (auto const named = name_at(document, encoding->child(Segment::entry("BaseEncoding")))) {
      base = *named;
    }
    differences = encoding->child(Segment::entry("Differences"));
  }
  result.simple_ = base_encoding(base);
  if (!result.simple_) {
    result.simple_.emplace();
    result.simple_->fill(charset::unicode::replacement_character);
  }
  if (differences) {
    // [code /name /name ... code /name ...]: names for consecutive codes
    long long code = 0;
    for (auto const& item : document.children(*differences)) {
      auto const bytes = item.content.bytes();
      if (item.content.kind() == Kind::integer) {
        std::from_chars(bytes.data(), bytes.data() + bytes.size(), code);
      } else if (item.content.kind() == Kind::name) {
        if (code >= 0 && code < 256) {
          (*result.simple_)[static_cast<std::size_t>(code)] =
              glyph_unicode(decode_name(bytes)).value_or(charset::unicode::replacement_character);
        }
        ++code;
      }
    }
  }
  return result;
}

std::vector<Font::Glyph> Font::glyphs(std::string_view codes) const {
  std::vector<Glyph> result;
  for (std::size_t at = 0; at < codes.size();) {
    auto length = code_bytes_;
    if (encoding_) {
      length = encoding_->code_length(codes.substr(at)).value_or(code_bytes_);
    }
    length = std::min(length, codes.size() - at);
    auto const code = codes.substr(at, length);
    at += length;
    result.push_back({code, utf8_of(code), width_of(code) / 1000});
  }
  return result;
}

std::string Font::text(std::string_view codes) const {
  std::string result;
  for (auto const& glyph : glyphs(codes)) {
    result += glyph.text;
  }
  return result;
}

std::string Font::utf8_of(std::string_view code) const {
  if (to_unicode_) {
    if (auto const text = to_unicode_->utf8(code)) {
      return *text;
    }
  }
  std::string result;
  if (simple_ && code.size() == 1) {
    append_utf8(result, (*simple_)[static_cast<unsigned char>(code[0])]);
  } else {
    append_utf8(result, charset::unicode::replacement_character);
  }
  return result;
}

double Font::width_of(std::string_view code) const {
  if (simple_ && code.size() == 1) { // a simple font
    return simple_widths_[static_cast<unsigned char>(code[0])];
  }
  std::uint32_t cid = 0; // Identity-H: the code is the CID
  for (auto const c : code) {
    cid = (cid << 8) | static_cast<unsigned char>(c);
  }
  auto const found = cid_widths_.find(cid);
  return found == cid_widths_.end() ? default_width_ : found->second;
}

} // namespace walk_pdf
