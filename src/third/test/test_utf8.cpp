#include "charset/cp437.hpp"
#include "charset/utf8.hpp"

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <vector>

using charset::utf8::Encoder;
using charset::utf8::Decoder;

namespace {

// The bytes of one push, as a string to compare against escapes
std::string encoded(char32_t code_point) {
  Encoder buffer;
  std::string result;
  for (auto const byte : buffer.push(code_point)) {
    result.push_back(static_cast<char>(byte));
  }
  return result;
}

// Every code point from pushing the bytes one at a time, then finishing
std::u32string decoded(Decoder& buffer, std::string_view bytes) {
  std::u32string result;
  for (char const c : bytes) {
    if (auto const code_point = buffer.push(static_cast<charset::utf8::CodePoint>(c))) {
      result.push_back(*code_point);
    }
  }
  if (auto const code_point = buffer.finish()) {
    result.push_back(*code_point);
  }
  return result;
}

std::u32string decoded(std::string_view bytes) {
  Decoder buffer;
  return decoded(buffer, bytes);
}

std::vector<charset::utf8::CodePoint> as_bytes(std::string_view bytes) {
  return {bytes.begin(), bytes.end()};
}

} // namespace

TEST(Utf8, AsciiIsOneByte) {
  EXPECT_EQ(encoded(0x00), std::string(1, '\0'));
  EXPECT_EQ(encoded(U'A'), "A");
  EXPECT_EQ(encoded(0x7F), "\x7F");
}

TEST(Utf8, TwoBytes) {
  EXPECT_EQ(encoded(0x80), "\xC2\x80");
  EXPECT_EQ(encoded(0xE4), "\xC3\xA4"); // ä
  EXPECT_EQ(encoded(0xC5), "\xC3\x85"); // Å
  EXPECT_EQ(encoded(0x7FF), "\xDF\xBF");
}

TEST(Utf8, ThreeBytes) {
  EXPECT_EQ(encoded(0x800), "\xE0\xA0\x80");
  EXPECT_EQ(encoded(0x20AC), "\xE2\x82\xAC"); // €
  EXPECT_EQ(encoded(0x2554), "\xE2\x95\x94"); // ╔
  EXPECT_EQ(encoded(0xFFFF), "\xEF\xBF\xBF");
}

TEST(Utf8, FourBytes) {
  EXPECT_EQ(encoded(0x10000), "\xF0\x90\x80\x80");
  EXPECT_EQ(encoded(0x1F600), "\xF0\x9F\x98\x80"); // 😀
  EXPECT_EQ(encoded(0x10FFFF), "\xF4\x8F\xBF\xBF");
}

TEST(Utf8, NonCharactersGiveReplacementCharacter) {
  EXPECT_EQ(encoded(0xD800), "\xEF\xBF\xBD");
  EXPECT_EQ(encoded(0xDFFF), "\xEF\xBF\xBD");
  EXPECT_EQ(encoded(0x110000), "\xEF\xBF\xBD");
}

TEST(Utf8, EncoderCountsNonCharacters) {
  charset::utf8::Encoder encoder;
  encoder.push(U'A');
  encoder.push(U'\U0001F600');
  encoder.push(0xFFFD); // a real character, though it stands in for others
  EXPECT_EQ(encoder.unmapped_count(), 0U);
  encoder.push(0xDFFF);
  encoder.push(0x110000);
  EXPECT_EQ(encoder.unmapped_count(), 2U);
}

// Each code point goes straight from one buffer into the other
TEST(Utf8, TranscodesCp437ToUtf8) {
  charset::cp437::Decoder to_unicode;
  Encoder from_unicode;
  std::string result;
  for (char const c : std::string_view{"\x8Fr \x84r \x94l\r\n"}) {
    if (auto const code_point = to_unicode.push(static_cast<charset::cp437::CodePoint>(c))) {
      for (auto const byte : from_unicode.push(*code_point)) {
        result.push_back(static_cast<char>(byte));
      }
    }
  }
  EXPECT_EQ(result, "\xC3\x85r \xC3\xA4r \xC3\xB6l\r\n"); // År är öl
}

TEST(Utf8ToUnicode, Ascii) {
  EXPECT_EQ(decoded(std::string_view{"\0A\x7F", 3}), std::u32string(U"\0A\x7F", 3));
}

TEST(Utf8ToUnicode, MultiByte) {
  EXPECT_EQ(decoded("\xC2\x80\xC3\xA4\xDF\xBF"), U"\x80\xE4\x7FF");               // ä
  EXPECT_EQ(decoded("\xE0\xA0\x80\xE2\x82\xAC\xEF\xBF\xBF"), U"\x800\x20AC\xFFFF");   // €
  EXPECT_EQ(decoded("\xF0\x90\x80\x80\xF0\x9F\x98\x80\xF4\x8F\xBF\xBF"),
            U"\x10000\x1F600\x10FFFF"); // 😀
}

TEST(Utf8ToUnicode, CodePointOnlyOnLastByte) {
  Decoder buffer;
  EXPECT_FALSE(buffer.push(0xE2));
  EXPECT_FALSE(buffer.push(0x82));
  EXPECT_EQ(buffer.push(0xAC), U'\x20AC');
  EXPECT_FALSE(buffer.finish());
}

TEST(Utf8ToUnicode, NoFailuresWhenWellFormed) {
  Decoder buffer;
  decoded(buffer, "\xC3\x85r \xC3\xA4r \xC3\xB6l");
  EXPECT_EQ(buffer.failure_count(), 0u);
  EXPECT_TRUE(buffer.failed_bytes().empty());
}

TEST(Utf8ToUnicode, BadLeadByteFailsAlone) {
  for (std::string const bad : {"\x80", "\xBF", "\xC0", "\xC1", "\xF5", "\xFF"}) {
    Decoder buffer;
    EXPECT_EQ(decoded(buffer, bad + "A"), U"\xFFFD" "A") << int(static_cast<unsigned char>(bad[0]));
    EXPECT_EQ(buffer.failure_count(), 1u);
    EXPECT_EQ(buffer.failed_bytes(), as_bytes(bad));
  }
}

// The byte that breaks a sequence is consumed with it, even when it is good on its own
TEST(Utf8ToUnicode, BreakingByteIsConsumed) {
  Decoder buffer;
  EXPECT_EQ(decoded(buffer, "\xE2\x82" "AB"), U"\xFFFD" "B");
  EXPECT_EQ(buffer.failure_count(), 1u);
  EXPECT_EQ(buffer.failed_bytes(), as_bytes("\xE2\x82" "A"));
}

TEST(Utf8ToUnicode, BreakingLeadByteIsConsumed) {
  Decoder buffer;
  EXPECT_EQ(decoded(buffer, "\xC3\xC3\xA4"), U"\xFFFD\xFFFD");
  EXPECT_EQ(buffer.failure_count(), 2u);
  EXPECT_EQ(buffer.failed_bytes(), as_bytes("\xC3\xC3\xA4"));
}

TEST(Utf8ToUnicode, OverlongFails) {
  Decoder buffer;
  EXPECT_EQ(decoded(buffer, "\xE0\x9F\xBF" "A"), U"\xFFFD\xFFFD" "A");
  EXPECT_EQ(decoded(buffer, "\xF0\x8F\xBF\xBF"), U"\xFFFD\xFFFD\xFFFD");
  EXPECT_EQ(buffer.failure_count(), 5u);
  EXPECT_EQ(buffer.failed_bytes(), as_bytes("\xE0\x9F\xBF\xF0\x8F\xBF\xBF"));
}

TEST(Utf8ToUnicode, SurrogateFails) {
  Decoder buffer;
  EXPECT_EQ(decoded(buffer, "\xED\x9F\xBF"), U"\xD7FF");
  EXPECT_EQ(decoded(buffer, "\xED\xA0"), U"\xFFFD");
  EXPECT_EQ(buffer.failure_count(), 1u);
  EXPECT_EQ(buffer.failed_bytes(), as_bytes("\xED\xA0"));
}

TEST(Utf8ToUnicode, AboveMaxFails) {
  Decoder buffer;
  EXPECT_EQ(decoded(buffer, "\xF4\x90"), U"\xFFFD");
  EXPECT_EQ(buffer.failure_count(), 1u);
  EXPECT_EQ(buffer.failed_bytes(), as_bytes("\xF4\x90"));
}

TEST(Utf8ToUnicode, FinishFailsUnfinishedSequence) {
  Decoder buffer;
  EXPECT_EQ(decoded(buffer, "A\xF0\x9F\x98"), U"A\xFFFD");
  EXPECT_EQ(buffer.failure_count(), 1u);
  EXPECT_EQ(buffer.failed_bytes(), as_bytes("\xF0\x9F\x98"));
  EXPECT_FALSE(buffer.finish());
}

TEST(Utf8ToUnicode, RoundTrip) {
  Encoder from_unicode;
  Decoder to_unicode;
  for (char32_t const code_point : {0x00, 0x7F, 0x80, 0x7FF, 0x800, 0xD7FF, 0xE000, 0xFFFD, 0xFFFF,
                                    0x10000, 0x1F600, 0x10FFFF}) {
    std::optional<char32_t> result;
    for (auto const byte : from_unicode.push(code_point)) {
      result = to_unicode.push(byte);
    }
    EXPECT_EQ(result, code_point);
  }
  EXPECT_EQ(to_unicode.failure_count(), 0u);
}
