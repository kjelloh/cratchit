#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace walk_pdf {

// A CMap (PDF 32000-1:2008, 9.10.3 ToUnicode CMaps, and 9.7.5): how a
// font's character codes split, and the text each code stands for.
// Codes are kept as their bytes; the text as UTF-16BE, as in the CMap.
class CMap {
public:
  // Reads a CMap's (decoded) stream data: its codespace ranges, bfchar and
  // bfrange mappings. Anything else in it is ignored.
  static CMap parse(std::string_view data);

  // Does it have codespace ranges, which say how long codes are?
  bool has_code_space() const { return !code_space_.empty(); }

  // How many bytes the code at the start of bytes is: the length of a
  // codespace range it is in. nullopt if it is in none.
  std::optional<std::size_t> code_length(std::string_view bytes) const;

  // The text a code (its bytes) stands for, as UTF-8; nullopt if not mapped
  std::optional<std::string> utf8(std::string_view code) const;

private:
  struct CodeSpace {
    std::string low;  // each byte of a code is between low's
    std::string high; // and high's byte at that position
  };
  struct Range {
    std::string low;  // codes from low
    std::string high; // to high, of the same length
    std::string text; // UTF-16BE of low's text; the next codes add 1 to its last code unit
  };

  std::vector<CodeSpace> code_space_;
  std::map<std::string, std::string> chars_; // code -> UTF-16BE text
  std::vector<Range> ranges_;
};

} // namespace walk_pdf
