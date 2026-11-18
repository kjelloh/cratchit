#include "charset/decoder.hpp"

#include <gtest/gtest.h>

using charset::Decoder;
using charset::Encoding;

TEST(Decoder, Utf8) {
  Decoder decoder(Encoding::utf8);
  EXPECT_FALSE(decoder.push(0xC3));
  EXPECT_EQ(decoder.push(0xA5), U'å'); // å
  EXPECT_FALSE(decoder.finish());
}

TEST(Decoder, Utf16le) {
  Decoder decoder(Encoding::utf16le);
  EXPECT_FALSE(decoder.push(0xE5));
  EXPECT_EQ(decoder.push(0x00), U'å');
}

TEST(Decoder, Utf16be) {
  Decoder decoder(Encoding::utf16be);
  EXPECT_FALSE(decoder.push(0x00));
  EXPECT_EQ(decoder.push(0xE5), U'å');
}

TEST(Decoder, Cp437) {
  Decoder decoder(Encoding::cp437);
  EXPECT_EQ(decoder.push(0x86), U'å');
  EXPECT_FALSE(decoder.finish()); // CP437 has no finish of its own
}

TEST(Decoder, CountsFailures) {
  Decoder decoder(Encoding::utf8);
  EXPECT_EQ(decoder.push(0xFF), charset::unicode::replacement_character);
  EXPECT_FALSE(decoder.push(0xC3));
  EXPECT_EQ(decoder.finish(), charset::unicode::replacement_character);
  EXPECT_EQ(decoder.failure_count(), 2U);
}
