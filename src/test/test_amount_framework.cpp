#include "test_amount_framework.hpp"
#include "test_atomics.hpp"
#include "logger/log.hpp"
#include "fiscal/amount/AmountFramework.hpp"
#include <gtest/gtest.h>

namespace tests::amount_framework {

  namespace to_amount_suite {

    // Basic parsing tests - currently supported formats
    TEST(ToAmountTests, ParsesPlainInteger) {
      auto result = to_amount("1538");
      ASSERT_TRUE(result.has_value());
      EXPECT_EQ(*result, Amount{1538});
    }

    TEST(ToAmountTests, ParsesNegativeInteger) {
      auto result = to_amount("-1538");
      ASSERT_TRUE(result.has_value());
      EXPECT_EQ(*result, Amount{-1538});
    }

    TEST(ToAmountTests, ParsesDecimalWithDot) {
      auto result = to_amount("1538.50");
      ASSERT_TRUE(result.has_value());
      EXPECT_EQ(*result, Amount{1538.50});
    }

    TEST(ToAmountTests, ParsesDecimalWithComma) {
      // Swedish/European format: comma as decimal separator
      auto result = to_amount("1538,50");
      ASSERT_TRUE(result.has_value());
      EXPECT_EQ(*result, Amount{1538.50});
    }

    TEST(ToAmountTests, ParsesNegativeDecimalWithComma) {
      auto result = to_amount("-1083,75");
      ASSERT_TRUE(result.has_value());
      EXPECT_EQ(*result, Amount{-1083.75});
    }

    // Swedish format with space as thousands separator - EXPECTED TO FAIL until to_amount is fixed
    TEST(ToAmountTests, ParsesSpaceThousandsSeparator) {
      // Swedish format: "1 538" means 1538
      auto result = to_amount("1 538");
      ASSERT_TRUE(result.has_value()) << "Should parse '1 538' as valid amount";
      EXPECT_EQ(*result, Amount{1538}) << "Expected 1538, not truncated to 1";
    }

    TEST(ToAmountTests, ParsesSpaceThousandsSeparatorWithDecimal) {
      // Swedish format: "1 538,50" means 1538.50
      auto result = to_amount("1 538,50");
      ASSERT_TRUE(result.has_value()) << "Should parse '1 538,50' as valid amount";
      EXPECT_EQ(*result, Amount{1538.50});
    }

    TEST(ToAmountTests, ParsesNegativeSpaceThousandsSeparator) {
      auto result = to_amount("-1 538");
      ASSERT_TRUE(result.has_value());
      EXPECT_EQ(*result, Amount{-1538});
    }

    TEST(ToAmountTests, ParsesLargeAmountWithSpaces) {
      // "1 234 567,89" = 1234567.89
      auto result = to_amount("1 234 567,89");
      ASSERT_TRUE(result.has_value());
      EXPECT_EQ(*result, Amount{1234567.89});
    }

    // English format with comma as thousands separator
    TEST(ToAmountTests, ParsesEnglishThousandsSeparator) {
      // English format: "1,538.50" means 1538.50
      auto result = to_amount("1,538.50");
      ASSERT_TRUE(result.has_value()) << "Should parse '1,538.50' as valid amount";
      EXPECT_EQ(*result, Amount{1538.50});
    }

    TEST(ToAmountTests, ParsesEnglishLargeAmount) {
      // "1,234,567.89"
      auto result = to_amount("1,234,567.89");
      ASSERT_TRUE(result.has_value());
      EXPECT_EQ(*result, Amount{1234567.89});
    }

    // Edge cases
    TEST(ToAmountTests, ParsesZero) {
      auto result = to_amount("0");
      ASSERT_TRUE(result.has_value());
      EXPECT_EQ(*result, Amount{0});
    }

    TEST(ToAmountTests, ParsesZeroWithDecimals) {
      auto result = to_amount("0,00");
      ASSERT_TRUE(result.has_value());
      EXPECT_EQ(*result, Amount{0});
    }

    TEST(ToAmountTests, RejectsEmptyString) {
      auto result = to_amount("");
      EXPECT_FALSE(result.has_value());
    }

    TEST(ToAmountTests, RejectsNonNumericString) {
      auto result = to_amount("abc");
      EXPECT_FALSE(result.has_value());
    }

    TEST(ToAmountTests, RejectsOnlySpaces) {
      auto result = to_amount("   ");
      EXPECT_FALSE(result.has_value());
    }

  } // namespace to_amount_suite

} // namespace tests::amount_framework
