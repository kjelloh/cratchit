#include "charset/adobe_standard.hpp"
#include "charset/macroman.hpp"
#include "charset/winansi.hpp"

#include <gtest/gtest.h>

TEST(WinAnsi, Latin1AndWindowsCharacters) {
  charset::winansi::Decoder decoder;
  EXPECT_EQ(decoder.push('A'), U'A');
  EXPECT_EQ(decoder.push(0xE4), U'ä'); // ä
  EXPECT_EQ(decoder.push(0x80), U'€'); // €
  EXPECT_EQ(decoder.push(0x91), U'‘'); // ‘
  EXPECT_EQ(decoder.push(0x96), U'–'); // –
  EXPECT_EQ(decoder.push(0x81), U'�'); // not defined
  EXPECT_EQ(decoder.push(0x0A), U'�'); // control characters are not defined
}

TEST(MacRoman, Characters) {
  charset::macroman::Decoder decoder;
  EXPECT_EQ(decoder.push('A'), U'A');
  EXPECT_EQ(decoder.push(0x8A), U'ä'); // ä
  EXPECT_EQ(decoder.push(0x8C), U'å'); // å
  EXPECT_EQ(decoder.push(0x9A), U'ö'); // ö
  EXPECT_EQ(decoder.push(0xD2), U'“'); // “
}

TEST(AdobeStandard, Characters) {
  charset::adobe_standard::Decoder decoder;
  EXPECT_EQ(decoder.push('A'), U'A');
  EXPECT_EQ(decoder.push(0x27), U'’'); // quoteright
  EXPECT_EQ(decoder.push(0x60), U'‘'); // quoteleft
  EXPECT_EQ(decoder.push(0xAE), U'ﬁ'); // fi
  EXPECT_EQ(decoder.push(0xFB), U'ß'); // germandbls
  EXPECT_EQ(decoder.push(0xE4), U'�'); // not defined
}
