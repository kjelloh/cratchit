#include "utf8.hpp"
#include "unicode.hpp"

constexpr const std::string REPLACEMENT_UTF8 = "\uFFFD";

std::vector<uint8_t> unicode_to_utf8(uint32_t cp) {
  std::vector<uint8_t> result{};
  // Thanks to https://sourceforge.net/p/utfcpp/code/HEAD/tree/v3_0/src/utf8.h
  if (!is_valid_unicode(cp)) {
    for (auto const& utf8_byte : REPLACEMENT_UTF8) result.push_back(utf8_byte);
  }
  else if (cp < 0x80)                   // one octet
      result.push_back(static_cast<uint8_t>(cp));
  else if (cp < 0x800) {                // two octets
      result.push_back(static_cast<uint8_t>((cp >> 6)            | 0xc0));
      result.push_back(static_cast<uint8_t>((cp & 0x3f)          | 0x80));
  }
  else if (cp < 0x10000) {              // three octets
      result.push_back(static_cast<uint8_t>((cp >> 12)           | 0xe0));
      result.push_back(static_cast<uint8_t>(((cp >> 6) & 0x3f)   | 0x80));
      result.push_back(static_cast<uint8_t>((cp & 0x3f)          | 0x80));
  }
  else {                                // four octets
      result.push_back(static_cast<uint8_t>((cp >> 18)           | 0xf0));
      result.push_back(static_cast<uint8_t>(((cp >> 12) & 0x3f)  | 0x80));
      result.push_back(static_cast<uint8_t>(((cp >> 6) & 0x3f)   | 0x80));
      result.push_back(static_cast<uint8_t>((cp & 0x3f)          | 0x80));
  }

  return result;
};
