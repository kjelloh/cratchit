#include "core/amount.hpp"

#include <gtest/gtest.h>

using cratchit::Amount;
using cratchit::format_amount;
using cratchit::parse_amount;
using cratchit::scaled;

TEST(Amount, ParsesSieAmounts) {
  EXPECT_EQ(parse_amount("13000"), 1300000);
  EXPECT_EQ(parse_amount("-6498.77"), -649877);
  EXPECT_EQ(parse_amount("1.5"), 150);
  EXPECT_EQ(parse_amount("1.85"), 185);
  EXPECT_EQ(parse_amount("0"), 0);
  EXPECT_EQ(parse_amount("-0.01"), -1);
  EXPECT_EQ(parse_amount("999999999999999.99"), 99999999999999999);
}

TEST(Amount, RejectsWhatSieDoesNotAllow) {
  EXPECT_FALSE(parse_amount(""));
  EXPECT_FALSE(parse_amount("-"));
  EXPECT_FALSE(parse_amount("+5"));
  EXPECT_FALSE(parse_amount("1,50"));
  EXPECT_FALSE(parse_amount("1.505"));
  EXPECT_FALSE(parse_amount("1."));
  EXPECT_FALSE(parse_amount(".5"));
  EXPECT_FALSE(parse_amount("1 000"));
  EXPECT_FALSE(parse_amount("1e3"));
  EXPECT_FALSE(parse_amount("1000000000000000")); // 16 digits
}

TEST(Amount, FormatsWithTwoDecimals) {
  EXPECT_EQ(format_amount(1300000), "13000.00");
  EXPECT_EQ(format_amount(-649877), "-6498.77");
  EXPECT_EQ(format_amount(-1), "-0.01");
  EXPECT_EQ(format_amount(0), "0.00");
  EXPECT_EQ(parse_amount(format_amount(-123456789)), -123456789);
}

TEST(Amount, ScaledRoundsHalfAwayFromZero) {
  EXPECT_EQ(scaled(89440, 124500, 111800), 99600); // 894.40 of 1118.00, scaled to 1245.00
  EXPECT_EQ(scaled(-111800, 124500, 111800), -124500);
  EXPECT_EQ(scaled(1, 1, 2), 1);   // 0.005 öre up
  EXPECT_EQ(scaled(-1, 1, 2), -1); // and down
  EXPECT_EQ(scaled(1, 1, 3), 0);
  EXPECT_EQ(scaled(1000, -50, 100), -500); // a negative amount flips the sign
  EXPECT_EQ(scaled(1000, -50, -100), 500);
}

TEST(Amount, ScaledLargeAmountsWithoutOverflow) {
  Amount const big = 99999999999999999; // the largest SIE amount
  EXPECT_EQ(scaled(big, big, big), big);
  EXPECT_EQ(scaled(big / 4, big, big), big / 4);
  EXPECT_EQ(scaled(3, big, big), 3);
}
