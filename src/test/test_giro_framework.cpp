#include "fiscal/SwedishGiroFramework.hpp"
#include "logger/log.hpp"
#include <gtest/gtest.h>

namespace tests::giro_framework {

  // Test suite for Bankgiro (BG) number validation
  namespace bg {

    // ============================================================================
    // Valid BG number parsing tests
    // ============================================================================

    TEST(BankgiroParsing, Parse7DigitRegularNumber) {
      logger::scope_logger log_raii{logger::development_trace, "Parse7DigitRegularNumber"};

      auto maybe_bg = giro::BG::to_bankgiro_number("123-4567");

      ASSERT_TRUE(maybe_bg.has_value());
      EXPECT_EQ(maybe_bg->account_number, "123-4567");
      EXPECT_EQ(maybe_bg->type, giro::BG::BankgiroType::Regular);
      EXPECT_FALSE(giro::BG::is_ninety_number(*maybe_bg));
    }

    TEST(BankgiroParsing, Parse8DigitRegularNumber) {
      logger::scope_logger log_raii{logger::development_trace, "Parse8DigitRegularNumber"};

      auto maybe_bg = giro::BG::to_bankgiro_number("1234-5678");

      ASSERT_TRUE(maybe_bg.has_value());
      EXPECT_EQ(maybe_bg->account_number, "1234-5678");
      EXPECT_EQ(maybe_bg->type, giro::BG::BankgiroType::Regular);
      EXPECT_FALSE(giro::BG::is_ninety_number(*maybe_bg));
    }

    TEST(BankgiroParsing, Parse90Number) {
      logger::scope_logger log_raii{logger::development_trace, "Parse90Number"};

      auto maybe_bg = giro::BG::to_bankgiro_number("90-12345");

      ASSERT_TRUE(maybe_bg.has_value());
      EXPECT_EQ(maybe_bg->account_number, "90-12345");
      EXPECT_EQ(maybe_bg->type, giro::BG::BankgiroType::Ninety);
      EXPECT_TRUE(giro::BG::is_ninety_number(*maybe_bg));
    }

    TEST(BankgiroParsing, Parse90NumberWithNonZeros) {
      logger::scope_logger log_raii{logger::development_trace, "Parse90NumberWithNonZeros"};

      auto maybe_bg = giro::BG::to_bankgiro_number("90-99999");

      ASSERT_TRUE(maybe_bg.has_value());
      EXPECT_EQ(maybe_bg->account_number, "90-99999");
      EXPECT_EQ(maybe_bg->type, giro::BG::BankgiroType::Ninety);
    }

    TEST(BankgiroParsing, ParseWithLeadingWhitespace) {
      logger::scope_logger log_raii{logger::development_trace, "ParseWithLeadingWhitespace"};

      auto maybe_bg = giro::BG::to_bankgiro_number("  123-4567");

      ASSERT_TRUE(maybe_bg.has_value());
      EXPECT_EQ(maybe_bg->account_number, "123-4567");
    }

    TEST(BankgiroParsing, ParseWithTrailingWhitespace) {
      logger::scope_logger log_raii{logger::development_trace, "ParseWithTrailingWhitespace"};

      auto maybe_bg = giro::BG::to_bankgiro_number("123-4567  ");

      ASSERT_TRUE(maybe_bg.has_value());
      EXPECT_EQ(maybe_bg->account_number, "123-4567");
    }

    TEST(BankgiroParsing, ParseWithSurroundingWhitespace) {
      logger::scope_logger log_raii{logger::development_trace, "ParseWithSurroundingWhitespace"};

      auto maybe_bg = giro::BG::to_bankgiro_number("  1234-5678  ");

      ASSERT_TRUE(maybe_bg.has_value());
      EXPECT_EQ(maybe_bg->account_number, "1234-5678");
    }

    // ============================================================================
    // Invalid BG number rejection tests - Missing or wrong dash
    // ============================================================================

    TEST(BankgiroParsing, RejectNoDash7Digits) {
      logger::scope_logger log_raii{logger::development_trace, "RejectNoDash7Digits"};

      auto maybe_bg = giro::BG::to_bankgiro_number("1234567");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, RejectNoDash8Digits) {
      logger::scope_logger log_raii{logger::development_trace, "RejectNoDash8Digits"};

      auto maybe_bg = giro::BG::to_bankgiro_number("12345678");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, RejectMultipleDashes) {
      logger::scope_logger log_raii{logger::development_trace, "RejectMultipleDashes"};

      auto maybe_bg = giro::BG::to_bankgiro_number("12-34-567");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, RejectWrongDashPosition7Digits) {
      logger::scope_logger log_raii{logger::development_trace, "RejectWrongDashPosition7Digits"};

      // 7 digits but dash at wrong position (should be XXX-XXXX, not XX-XXXXX)
      auto maybe_bg = giro::BG::to_bankgiro_number("12-34567");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, RejectWrongDashPosition8Digits) {
      logger::scope_logger log_raii{logger::development_trace, "RejectWrongDashPosition8Digits"};

      // 8 digits but dash at wrong position (should be XXXX-XXXX, not XXX-XXXXX)
      auto maybe_bg = giro::BG::to_bankgiro_number("123-45678");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    // ============================================================================
    // Invalid BG number rejection tests - Wrong total digit count
    // ============================================================================

    TEST(BankgiroParsing, RejectTooFewDigits) {
      logger::scope_logger log_raii{logger::development_trace, "RejectTooFewDigits"};

      auto maybe_bg = giro::BG::to_bankgiro_number("12-3456");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, RejectTooManyDigits) {
      logger::scope_logger log_raii{logger::development_trace, "RejectTooManyDigits"};

      auto maybe_bg = giro::BG::to_bankgiro_number("1234-56789");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, Reject6Digits) {
      logger::scope_logger log_raii{logger::development_trace, "Reject6Digits"};

      auto maybe_bg = giro::BG::to_bankgiro_number("123-456");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, Reject9Digits) {
      logger::scope_logger log_raii{logger::development_trace, "Reject9Digits"};

      auto maybe_bg = giro::BG::to_bankgiro_number("12345-6789");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    // ============================================================================
    // Invalid BG number rejection tests - All-zero suffix
    // ============================================================================

    TEST(BankgiroParsing, RejectAllZeroSuffix7Digits) {
      logger::scope_logger log_raii{logger::development_trace, "RejectAllZeroSuffix7Digits"};

      auto maybe_bg = giro::BG::to_bankgiro_number("123-0000");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, RejectAllZeroSuffix8Digits) {
      logger::scope_logger log_raii{logger::development_trace, "RejectAllZeroSuffix8Digits"};

      auto maybe_bg = giro::BG::to_bankgiro_number("1234-0000");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, Reject90NumberWithAllZeros) {
      logger::scope_logger log_raii{logger::development_trace, "Reject90NumberWithAllZeros"};

      auto maybe_bg = giro::BG::to_bankgiro_number("90-00000");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    // ============================================================================
    // Invalid BG number rejection tests - 90-number specific
    // ============================================================================

    TEST(BankgiroParsing, Reject90NumberTooFewDigits) {
      logger::scope_logger log_raii{logger::development_trace, "Reject90NumberTooFewDigits"};

      auto maybe_bg = giro::BG::to_bankgiro_number("90-1234");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, Reject90NumberTooManyDigits) {
      logger::scope_logger log_raii{logger::development_trace, "Reject90NumberTooManyDigits"};

      auto maybe_bg = giro::BG::to_bankgiro_number("90-123456");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, Reject90NumberEmptySuffix) {
      logger::scope_logger log_raii{logger::development_trace, "Reject90NumberEmptySuffix"};

      auto maybe_bg = giro::BG::to_bankgiro_number("90-");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    // ============================================================================
    // Invalid BG number rejection tests - Non-numeric and empty
    // ============================================================================

    TEST(BankgiroParsing, RejectNonNumericPrefix) {
      logger::scope_logger log_raii{logger::development_trace, "RejectNonNumericPrefix"};

      auto maybe_bg = giro::BG::to_bankgiro_number("ABC-4567");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, RejectNonNumericSuffix) {
      logger::scope_logger log_raii{logger::development_trace, "RejectNonNumericSuffix"};

      auto maybe_bg = giro::BG::to_bankgiro_number("123-ABCD");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, RejectEmptyString) {
      logger::scope_logger log_raii{logger::development_trace, "RejectEmptyString"};

      auto maybe_bg = giro::BG::to_bankgiro_number("");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, RejectWhitespaceOnly) {
      logger::scope_logger log_raii{logger::development_trace, "RejectWhitespaceOnly"};

      auto maybe_bg = giro::BG::to_bankgiro_number("   ");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, RejectDashOnly) {
      logger::scope_logger log_raii{logger::development_trace, "RejectDashOnly"};

      auto maybe_bg = giro::BG::to_bankgiro_number("-");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    // ============================================================================
    // Canonical format tests
    // ============================================================================

    TEST(BankgiroCanonical, FormatRegular7Digit) {
      logger::scope_logger log_raii{logger::development_trace, "FormatRegular7Digit"};

      auto maybe_bg = giro::BG::to_bankgiro_number("123-4567");
      ASSERT_TRUE(maybe_bg.has_value());

      std::string canonical = giro::BG::to_canonical_bg(*maybe_bg);

      EXPECT_EQ(canonical, "123-4567");
    }

    TEST(BankgiroCanonical, FormatRegular8Digit) {
      logger::scope_logger log_raii{logger::development_trace, "FormatRegular8Digit"};

      auto maybe_bg = giro::BG::to_bankgiro_number("1234-5678");
      ASSERT_TRUE(maybe_bg.has_value());

      std::string canonical = giro::BG::to_canonical_bg(*maybe_bg);

      EXPECT_EQ(canonical, "1234-5678");
    }

    TEST(BankgiroCanonical, Format90Number) {
      logger::scope_logger log_raii{logger::development_trace, "Format90Number"};

      auto maybe_bg = giro::BG::to_bankgiro_number("90-12345");
      ASSERT_TRUE(maybe_bg.has_value());

      std::string canonical = giro::BG::to_canonical_bg(*maybe_bg);

      EXPECT_EQ(canonical, "90-12345");
    }

    TEST(BankgiroCanonical, FormatPreservesWhitespaceTrimming) {
      logger::scope_logger log_raii{logger::development_trace, "FormatPreservesWhitespaceTrimming"};

      auto maybe_bg = giro::BG::to_bankgiro_number("  123-4567  ");
      ASSERT_TRUE(maybe_bg.has_value());

      std::string canonical = giro::BG::to_canonical_bg(*maybe_bg);

      EXPECT_EQ(canonical, "123-4567");  // Whitespace should be trimmed
    }

    // ============================================================================
    // Type classification tests
    // ============================================================================

    TEST(BankgiroType, IdentifyRegular7Digit) {
      logger::scope_logger log_raii{logger::development_trace, "IdentifyRegular7Digit"};

      auto maybe_bg = giro::BG::to_bankgiro_number("123-4567");
      ASSERT_TRUE(maybe_bg.has_value());

      EXPECT_EQ(giro::BG::get_type(*maybe_bg), giro::BG::BankgiroType::Regular);
      EXPECT_FALSE(giro::BG::is_ninety_number(*maybe_bg));
    }

    TEST(BankgiroType, IdentifyRegular8Digit) {
      logger::scope_logger log_raii{logger::development_trace, "IdentifyRegular8Digit"};

      auto maybe_bg = giro::BG::to_bankgiro_number("1234-5678");
      ASSERT_TRUE(maybe_bg.has_value());

      EXPECT_EQ(giro::BG::get_type(*maybe_bg), giro::BG::BankgiroType::Regular);
      EXPECT_FALSE(giro::BG::is_ninety_number(*maybe_bg));
    }

    TEST(BankgiroType, Identify90Number) {
      logger::scope_logger log_raii{logger::development_trace, "Identify90Number"};

      auto maybe_bg = giro::BG::to_bankgiro_number("90-12345");
      ASSERT_TRUE(maybe_bg.has_value());

      EXPECT_EQ(giro::BG::get_type(*maybe_bg), giro::BG::BankgiroType::Ninety);
      EXPECT_TRUE(giro::BG::is_ninety_number(*maybe_bg));
    }

  } // namespace bg

} // namespace tests::giro_framework
