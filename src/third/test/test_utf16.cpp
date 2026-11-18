#include "charset/utf16.hpp"

#include <gtest/gtest.h>

#include <string>
#include <string_view>

using charset::utf16::ByteOrder;
using charset::utf16::Encoder;
using charset::utf16::Decoder;

namespace {

// The bytes of one push, as a string to compare against escapes
std::string encoded(char32_t code_point, ByteOrder order = ByteOrder::big_endian) {
  Encoder buffer{order};
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
    if (auto const code_point = buffer.push(static_cast<charset::utf16::Byte>(c))) {
      result.push_back(*code_point);
    }
  }
  if (auto const code_point = buffer.finish()) {
    result.push_back(*code_point);
  }
  return result;
}

std::u32string decoded(std::string_view bytes, ByteOrder order = ByteOrder::big_endian) {
  Decoder buffer{order};
  return decoded(buffer, bytes);
}

using namespace std::string_view_literals;

} // namespace

TEST(Utf16, EncodesBasicPlaneAsOneCodeUnit) {
  EXPECT_EQ(encoded(U'A'), "\x00\x41"sv);
  EXPECT_EQ(encoded(U'ö'), "\x00\xF6"sv); // ö
  EXPECT_EQ(encoded(U'€'), "\x20\xAC"sv); // €
  EXPECT_EQ(encoded(U'￿'), "\xFF\xFF"sv);
}

TEST(Utf16, EncodesLittleEndian) {
  EXPECT_EQ(encoded(U'A', ByteOrder::little_endian), "\x41\x00"sv);
  EXPECT_EQ(encoded(U'€', ByteOrder::little_endian), "\xAC\x20"sv);
  EXPECT_EQ(encoded(U'\U0001F600', ByteOrder::little_endian), "\x3D\xD8\x00\xDE"sv);
}

TEST(Utf16, EncodesAboveBasicPlaneAsSurrogatePair) {
  EXPECT_EQ(encoded(U'\U00010000'), "\xD8\x00\xDC\x00"sv);
  EXPECT_EQ(encoded(U'\U0001F600'), "\xD8\x3D\xDE\x00"sv); // 😀
  EXPECT_EQ(encoded(U'\U0010FFFF'), "\xDB\xFF\xDF\xFF"sv);
}

TEST(Utf16, EncodesNonCharactersAsReplacementCharacter) {
  EXPECT_EQ(encoded(0xD800), "\xFF\xFD"sv);
  EXPECT_EQ(encoded(0xDFFF), "\xFF\xFD"sv);
  EXPECT_EQ(encoded(0x110000), "\xFF\xFD"sv);
}

TEST(Utf16, EncoderCountsNonCharacters) {
  charset::utf16::Encoder encoder;
  encoder.push(U'A');
  encoder.push(U'\U0001F600');
  encoder.push(0xFFFD); // a real character, though it stands in for others
  EXPECT_EQ(encoder.unmapped_count(), 0U);
  encoder.push(0xD800);
  encoder.push(0x110000);
  EXPECT_EQ(encoder.unmapped_count(), 2U);
}

TEST(Utf16, DecodesBigEndian) {
  EXPECT_EQ(decoded("\x00I\x00n\x00v\x00o\x00i\x00\x63\x00\x65"sv), U"Invoice");
  EXPECT_EQ(decoded("\x00M\x00\x61\x00l\x00m\x00\xF6"sv), U"Malmö");
  EXPECT_EQ(decoded("\xD8\x3D\xDE\x00"sv), U"\U0001F600");
}

TEST(Utf16, DecodesLittleEndian) {
  EXPECT_EQ(decoded("I\x00n\x00"sv, ByteOrder::little_endian), U"In");
  EXPECT_EQ(decoded("\x3D\xD8\x00\xDE"sv, ByteOrder::little_endian), U"\U0001F600");
}

TEST(Utf16, RoundTrip) {
  for (char32_t const code_point : {U'\0', U'A', U'å', U'߿', U'퟿', U'', U'�',
                                    U'\U00010000', U'\U0001F600', U'\U0010FFFF'}) {
    for (auto const order : {ByteOrder::big_endian, ByteOrder::little_endian}) {
      EXPECT_EQ(decoded(encoded(code_point, order), order), std::u32string(1, code_point));
    }
  }
}

TEST(Utf16, LoneHighSurrogateFailsWithTheUnitThatBrokeIt) {
  // As in UTF-8: the broken sequence and the unit that broke it give one U+FFFD
  Decoder buffer;
  EXPECT_EQ(decoded(buffer, "\xD8\x3D\x00\x41\x00\x42"sv), U"�" U"B");
  EXPECT_EQ(buffer.failure_count(), 1U);
}

TEST(Utf16, TwoHighSurrogatesInARow) {
  // The second breaks the first; both are consumed
  EXPECT_EQ(decoded("\xD8\x3D\xD8\x3D\xDE\x00\x00\x41"sv), U"��" U"A");
}

TEST(Utf16, EveryPushGivesAtMostOneCodePoint) {
  Decoder buffer;
  EXPECT_FALSE(buffer.push(0xD8));
  EXPECT_FALSE(buffer.push(0x3D)); // a high surrogate: waits for its partner
  EXPECT_FALSE(buffer.push(0xDE));
  EXPECT_EQ(buffer.push(0x00), U'\U0001F600');
}

TEST(Utf16, LoneLowSurrogateFails) {
  Decoder buffer;
  EXPECT_EQ(decoded(buffer, "\xDE\x00\x00\x41"sv), U"�" U"A");
  EXPECT_EQ(buffer.failure_count(), 1U);
}

TEST(Utf16, UnfinishedInputFailsAtFinish) {
  Decoder odd;
  EXPECT_EQ(decoded(odd, "\x00\x41\x00"sv), U"A�");
  EXPECT_EQ(odd.failure_count(), 1U);
  Decoder high;
  EXPECT_EQ(decoded(high, "\x00\x41\xD8\x3D"sv), U"A�");
  EXPECT_EQ(high.failure_count(), 1U);
}

TEST(Utf16, NothingPushedFinishesWithNothing) {
  Decoder buffer;
  EXPECT_FALSE(buffer.finish());
  EXPECT_EQ(buffer.failure_count(), 0U);
}
