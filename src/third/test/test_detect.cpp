#include "charset/detect.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <string_view>
#include <vector>

namespace {

std::vector<std::uint8_t> as_bytes(std::string_view text) {
  return {text.begin(), text.end()};
}

} // namespace

TEST(CharsetDetect, Utf8NeedsAMultiByteCharacter) {
  EXPECT_TRUE(charset::detect::is_utf8(as_bytes("#FNAMN \"Sm\xc3\xa5 AB\"\r\n")));
  EXPECT_FALSE(charset::detect::is_utf8(as_bytes("#FNAMN \"Small AB\"\r\n"))); // ASCII tells nothing
  EXPECT_FALSE(charset::detect::is_utf8(as_bytes("")));
}

TEST(CharsetDetect, Utf8MustBeWellFormed) {
  EXPECT_FALSE(charset::detect::is_utf8(as_bytes("Sm\xc3\xa5 \x86")));   // CP437 å after a good one
  EXPECT_FALSE(charset::detect::is_utf8(as_bytes("Sm\xe5")));            // Windows-1252 å
  EXPECT_FALSE(charset::detect::is_utf8(as_bytes("Sm\xc3\xa5 \xe2\x82"))); // cut at the end
}
