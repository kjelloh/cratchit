#include "charset/cp437.hpp"

#include <gtest/gtest.h>

#include <set>

using charset::cp437::CodePoint;
using charset::cp437::Decoder;

TEST(Cp437, AsciiIncludingControlCharactersIsUnchanged) {
  Decoder buffer;
  for (unsigned byte = 0x00; byte < 0x80; ++byte) {
    EXPECT_EQ(buffer.push(static_cast<CodePoint>(byte)), char32_t(byte)) << "byte " << byte;
  }
}

TEST(Cp437, SwedishLetters) {
  Decoder buffer;
  EXPECT_EQ(buffer.push(0x86), U'å');
  EXPECT_EQ(buffer.push(0x84), U'ä');
  EXPECT_EQ(buffer.push(0x94), U'ö');
  EXPECT_EQ(buffer.push(0x8F), U'Å');
  EXPECT_EQ(buffer.push(0x8E), U'Ä');
  EXPECT_EQ(buffer.push(0x99), U'Ö');
  EXPECT_EQ(buffer.push(0x82), U'é');
}

TEST(Cp437, SymbolsAndBoxDrawing) {
  Decoder buffer;
  EXPECT_EQ(buffer.push(0x9E), U'₧'); // ₧
  EXPECT_EQ(buffer.push(0xB0), U'░'); // ░
  EXPECT_EQ(buffer.push(0xC9), U'╔'); // ╔
  EXPECT_EQ(buffer.push(0xDB), U'█'); // █
  EXPECT_EQ(buffer.push(0xE1), U'ß'); // ß
  EXPECT_EQ(buffer.push(0xFF), U' '); // no-break space
}

TEST(Cp437, EveryByteGivesADistinctCodePoint) {
  Decoder buffer;
  std::set<char32_t> seen;
  for (unsigned byte = 0x00; byte <= 0xFF; ++byte) {
    auto const code_point = buffer.push(static_cast<CodePoint>(byte));
    ASSERT_TRUE(code_point.has_value()) << "byte " << byte;
    EXPECT_TRUE(seen.insert(*code_point).second) << "byte " << byte;
  }
  EXPECT_EQ(seen.size(), 256u);
}

TEST(Cp437, EveryByteRoundTrips) {
  charset::cp437::Decoder to_unicode;
  charset::cp437::Encoder from_unicode;
  for (unsigned byte = 0; byte < 256; ++byte) {
    auto const code_point = to_unicode.push(static_cast<charset::cp437::CodePoint>(byte));
    ASSERT_TRUE(code_point.has_value());
    EXPECT_EQ(from_unicode.push(*code_point), static_cast<charset::cp437::CodePoint>(byte)) << byte;
  }
}

TEST(Cp437, CodePointsOutsideCp437GiveNothing) {
  charset::cp437::Encoder from_unicode;
  EXPECT_FALSE(from_unicode.push(0x20AC).has_value()); // euro sign
  EXPECT_FALSE(from_unicode.push(0x00C3).has_value()); // A tilde
}

TEST(Cp437, EncoderCountsUnmappedCodePoints) {
  charset::cp437::Encoder from_unicode;
  EXPECT_EQ(from_unicode.unmapped_count(), 0U);
  from_unicode.push(U'\u00E5');  // å, in CP437
  from_unicode.push(U'A');
  EXPECT_EQ(from_unicode.unmapped_count(), 0U);
  from_unicode.push(0x20AC);      // euro sign, not in CP437
  from_unicode.push(0xFFFD);      // replacement character, not in CP437
  EXPECT_EQ(from_unicode.unmapped_count(), 2U);
}

TEST(Cp437, DecoderNeverFails) {
  charset::cp437::Decoder to_unicode;
  for (int byte = 0; byte < 256; ++byte) to_unicode.push(static_cast<charset::cp437::CodePoint>(byte));
  EXPECT_EQ(to_unicode.failure_count(), 0U);
}
