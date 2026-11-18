#pragma once

#include "charset/unicode.hpp"
#include "cmap.hpp"
#include "path.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace walk_pdf {

class Document;

// What a font says about turning the bytes of its strings into text
// (PDF 32000-1:2008, 9.5 to 9.10): how they split into character codes, and
// the Unicode each code stands for.
class Font {
public:
  // Reads the font at path: its /Subtype, /Encoding and /ToUnicode
  static Font load(Document const& document, Path const& font);

  // The text that a string's bytes show in this font, as UTF-8. Codes are
  // mapped by the font's /ToUnicode map, or else (simple fonts) by its
  // /Encoding. A code that maps to nothing gives U+FFFD.
  std::string text(std::string_view codes) const;

  // One character code of a string: its bytes, the text it stands for (as
  // text() maps it), and how far it moves the text position, in text space
  // units (multiply by the font size)
  struct Glyph {
    std::string_view code;
    std::string text;
    double width;
  };
  std::vector<Glyph> glyphs(std::string_view codes) const;

  std::size_t code_bytes() const { return code_bytes_; }
  bool has_widths() const { return has_widths_; }
  bool has_to_unicode() const { return to_unicode_.has_value(); }

private:
  std::size_t code_bytes_ = 1;          // 1 for simple fonts; 2 for Type0 with Identity-H/V
  std::optional<CMap> encoding_;        // a Type0 font's embedded CMap, for its code lengths
  std::optional<CMap> to_unicode_;      // /ToUnicode
  // A simple font's code -> code point, from its /Encoding (U+FFFD: not mapped)
  std::optional<std::array<charset::unicode::CodePoint, 256>> simple_;

  // Widths, in thousandths of text space (glyph space of Type 3 fonts converted)
  std::string utf8_of(std::string_view code) const;
  double width_of(std::string_view code) const;
  bool has_widths_ = false;
  std::array<double, 256> simple_widths_{}; // simple fonts, by code
  std::map<std::uint32_t, double> cid_widths_; // Type0, by CID (the code, for Identity-H)
  double default_width_ = 500;                // codes without a width (an estimate if none is given)
};

} // namespace walk_pdf
