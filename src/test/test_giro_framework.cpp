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

      auto maybe_bg = account::giro::BG::to_bankgiro_number("123-4567");

      ASSERT_TRUE(maybe_bg.has_value());
      EXPECT_EQ(maybe_bg->account_number, "123-4567");
      EXPECT_EQ(maybe_bg->type, account::giro::BG::BankgiroType::Regular);
      EXPECT_FALSE(account::giro::BG::is_ninety_number(*maybe_bg));
    }

    TEST(BankgiroParsing, Parse8DigitRegularNumber) {
      logger::scope_logger log_raii{logger::development_trace, "Parse8DigitRegularNumber"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("1234-5678");

      ASSERT_TRUE(maybe_bg.has_value());
      EXPECT_EQ(maybe_bg->account_number, "1234-5678");
      EXPECT_EQ(maybe_bg->type, account::giro::BG::BankgiroType::Regular);
      EXPECT_FALSE(account::giro::BG::is_ninety_number(*maybe_bg));
    }

    TEST(BankgiroParsing, Parse90Number) {
      logger::scope_logger log_raii{logger::development_trace, "Parse90Number"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("90-12345");

      ASSERT_TRUE(maybe_bg.has_value());
      EXPECT_EQ(maybe_bg->account_number, "90-12345");
      EXPECT_EQ(maybe_bg->type, account::giro::BG::BankgiroType::Ninety);
      EXPECT_TRUE(account::giro::BG::is_ninety_number(*maybe_bg));
    }

    TEST(BankgiroParsing, Parse90NumberWithNonZeros) {
      logger::scope_logger log_raii{logger::development_trace, "Parse90NumberWithNonZeros"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("90-99999");

      ASSERT_TRUE(maybe_bg.has_value());
      EXPECT_EQ(maybe_bg->account_number, "90-99999");
      EXPECT_EQ(maybe_bg->type, account::giro::BG::BankgiroType::Ninety);
    }

    TEST(BankgiroParsing, ParseWithLeadingWhitespace) {
      logger::scope_logger log_raii{logger::development_trace, "ParseWithLeadingWhitespace"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("  123-4567");

      ASSERT_TRUE(maybe_bg.has_value());
      EXPECT_EQ(maybe_bg->account_number, "123-4567");
    }

    TEST(BankgiroParsing, ParseWithTrailingWhitespace) {
      logger::scope_logger log_raii{logger::development_trace, "ParseWithTrailingWhitespace"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("123-4567  ");

      ASSERT_TRUE(maybe_bg.has_value());
      EXPECT_EQ(maybe_bg->account_number, "123-4567");
    }

    TEST(BankgiroParsing, ParseWithSurroundingWhitespace) {
      logger::scope_logger log_raii{logger::development_trace, "ParseWithSurroundingWhitespace"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("  1234-5678  ");

      ASSERT_TRUE(maybe_bg.has_value());
      EXPECT_EQ(maybe_bg->account_number, "1234-5678");
    }

    // ============================================================================
    // Invalid BG number rejection tests - Missing or wrong dash
    // ============================================================================

    TEST(BankgiroParsing, RejectNoDash7Digits) {
      logger::scope_logger log_raii{logger::development_trace, "RejectNoDash7Digits"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("1234567");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, RejectNoDash8Digits) {
      logger::scope_logger log_raii{logger::development_trace, "RejectNoDash8Digits"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("12345678");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, RejectMultipleDashes) {
      logger::scope_logger log_raii{logger::development_trace, "RejectMultipleDashes"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("12-34-567");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, RejectWrongDashPosition7Digits) {
      logger::scope_logger log_raii{logger::development_trace, "RejectWrongDashPosition7Digits"};

      // 7 digits but dash at wrong position (should be XXX-XXXX, not XX-XXXXX)
      auto maybe_bg = account::giro::BG::to_bankgiro_number("12-34567");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, RejectWrongDashPosition8Digits) {
      logger::scope_logger log_raii{logger::development_trace, "RejectWrongDashPosition8Digits"};

      // 8 digits but dash at wrong position (should be XXXX-XXXX, not XXX-XXXXX)
      auto maybe_bg = account::giro::BG::to_bankgiro_number("123-45678");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    // ============================================================================
    // Invalid BG number rejection tests - Wrong total digit count
    // ============================================================================

    TEST(BankgiroParsing, RejectTooFewDigits) {
      logger::scope_logger log_raii{logger::development_trace, "RejectTooFewDigits"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("12-3456");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, RejectTooManyDigits) {
      logger::scope_logger log_raii{logger::development_trace, "RejectTooManyDigits"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("1234-56789");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, Reject6Digits) {
      logger::scope_logger log_raii{logger::development_trace, "Reject6Digits"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("123-456");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, Reject9Digits) {
      logger::scope_logger log_raii{logger::development_trace, "Reject9Digits"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("12345-6789");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    // ============================================================================
    // Invalid BG number rejection tests - All-zero suffix
    // ============================================================================

    TEST(BankgiroParsing, RejectAllZeroSuffix7Digits) {
      logger::scope_logger log_raii{logger::development_trace, "RejectAllZeroSuffix7Digits"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("123-0000");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, RejectAllZeroSuffix8Digits) {
      logger::scope_logger log_raii{logger::development_trace, "RejectAllZeroSuffix8Digits"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("1234-0000");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, Reject90NumberWithAllZeros) {
      logger::scope_logger log_raii{logger::development_trace, "Reject90NumberWithAllZeros"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("90-00000");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    // ============================================================================
    // Invalid BG number rejection tests - 90-number specific
    // ============================================================================

    TEST(BankgiroParsing, Reject90NumberTooFewDigits) {
      logger::scope_logger log_raii{logger::development_trace, "Reject90NumberTooFewDigits"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("90-1234");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, Reject90NumberTooManyDigits) {
      logger::scope_logger log_raii{logger::development_trace, "Reject90NumberTooManyDigits"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("90-123456");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, Reject90NumberEmptySuffix) {
      logger::scope_logger log_raii{logger::development_trace, "Reject90NumberEmptySuffix"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("90-");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    // ============================================================================
    // Invalid BG number rejection tests - Non-numeric and empty
    // ============================================================================

    TEST(BankgiroParsing, RejectNonNumericPrefix) {
      logger::scope_logger log_raii{logger::development_trace, "RejectNonNumericPrefix"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("ABC-4567");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, RejectNonNumericSuffix) {
      logger::scope_logger log_raii{logger::development_trace, "RejectNonNumericSuffix"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("123-ABCD");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, RejectEmptyString) {
      logger::scope_logger log_raii{logger::development_trace, "RejectEmptyString"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, RejectWhitespaceOnly) {
      logger::scope_logger log_raii{logger::development_trace, "RejectWhitespaceOnly"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("   ");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    TEST(BankgiroParsing, RejectDashOnly) {
      logger::scope_logger log_raii{logger::development_trace, "RejectDashOnly"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("-");

      EXPECT_FALSE(maybe_bg.has_value());
    }

    // ============================================================================
    // Canonical format tests
    // ============================================================================

    TEST(BankgiroCanonical, FormatRegular7Digit) {
      logger::scope_logger log_raii{logger::development_trace, "FormatRegular7Digit"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("123-4567");
      ASSERT_TRUE(maybe_bg.has_value());

      std::string canonical = account::giro::BG::to_canonical_bg(*maybe_bg);

      EXPECT_EQ(canonical, "123-4567");
    }

    TEST(BankgiroCanonical, FormatRegular8Digit) {
      logger::scope_logger log_raii{logger::development_trace, "FormatRegular8Digit"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("1234-5678");
      ASSERT_TRUE(maybe_bg.has_value());

      std::string canonical = account::giro::BG::to_canonical_bg(*maybe_bg);

      EXPECT_EQ(canonical, "1234-5678");
    }

    TEST(BankgiroCanonical, Format90Number) {
      logger::scope_logger log_raii{logger::development_trace, "Format90Number"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("90-12345");
      ASSERT_TRUE(maybe_bg.has_value());

      std::string canonical = account::giro::BG::to_canonical_bg(*maybe_bg);

      EXPECT_EQ(canonical, "90-12345");
    }

    TEST(BankgiroCanonical, FormatPreservesWhitespaceTrimming) {
      logger::scope_logger log_raii{logger::development_trace, "FormatPreservesWhitespaceTrimming"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("  123-4567  ");
      ASSERT_TRUE(maybe_bg.has_value());

      std::string canonical = account::giro::BG::to_canonical_bg(*maybe_bg);

      EXPECT_EQ(canonical, "123-4567");  // Whitespace should be trimmed
    }

    // ============================================================================
    // Type classification tests
    // ============================================================================

    TEST(BankgiroType, IdentifyRegular7Digit) {
      logger::scope_logger log_raii{logger::development_trace, "IdentifyRegular7Digit"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("123-4567");
      ASSERT_TRUE(maybe_bg.has_value());

      EXPECT_EQ(account::giro::BG::get_type(*maybe_bg), account::giro::BG::BankgiroType::Regular);
      EXPECT_FALSE(account::giro::BG::is_ninety_number(*maybe_bg));
    }

    TEST(BankgiroType, IdentifyRegular8Digit) {
      logger::scope_logger log_raii{logger::development_trace, "IdentifyRegular8Digit"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("1234-5678");
      ASSERT_TRUE(maybe_bg.has_value());

      EXPECT_EQ(account::giro::BG::get_type(*maybe_bg), account::giro::BG::BankgiroType::Regular);
      EXPECT_FALSE(account::giro::BG::is_ninety_number(*maybe_bg));
    }

    TEST(BankgiroType, Identify90Number) {
      logger::scope_logger log_raii{logger::development_trace, "Identify90Number"};

      auto maybe_bg = account::giro::BG::to_bankgiro_number("90-12345");
      ASSERT_TRUE(maybe_bg.has_value());

      EXPECT_EQ(account::giro::BG::get_type(*maybe_bg), account::giro::BG::BankgiroType::Ninety);
      EXPECT_TRUE(account::giro::BG::is_ninety_number(*maybe_bg));
    }

  } // namespace bg

  // Test suite for Plusgiro (PG) number validation
  namespace pg {

    // ============================================================================
    // Valid PG number parsing tests
    // ============================================================================

    TEST(PlusgiroParsing, Parse5DigitNumber) {
      logger::scope_logger log_raii{logger::development_trace, "Parse5DigitNumber"};

      // Calculate valid check digit for "1234"
      char check = account::giro::PG::calculate_check_digit("1234");
      std::string valid_pg = std::string("1234-") + check;

      auto maybe_pg = account::giro::PG::to_plusgiro_number(valid_pg);

      ASSERT_TRUE(maybe_pg.has_value());
      EXPECT_EQ(maybe_pg->number, "1234");
      EXPECT_EQ(maybe_pg->check_digit, check);
      EXPECT_EQ(maybe_pg->to_string(), valid_pg);
    }

    TEST(PlusgiroParsing, Parse6DigitNumber) {
      logger::scope_logger log_raii{logger::development_trace, "Parse6DigitNumber"};

      char check = account::giro::PG::calculate_check_digit("12345");
      std::string valid_pg = std::string("12345-") + check;

      auto maybe_pg = account::giro::PG::to_plusgiro_number(valid_pg);

      ASSERT_TRUE(maybe_pg.has_value());
      EXPECT_EQ(maybe_pg->number, "12345");
      EXPECT_EQ(maybe_pg->check_digit, check);
      EXPECT_EQ(maybe_pg->to_string(), valid_pg);
    }

    TEST(PlusgiroParsing, Parse7DigitNumber) {
      logger::scope_logger log_raii{logger::development_trace, "Parse7DigitNumber"};

      char check = account::giro::PG::calculate_check_digit("123456");
      std::string valid_pg = std::string("123456-") + check;

      auto maybe_pg = account::giro::PG::to_plusgiro_number(valid_pg);

      ASSERT_TRUE(maybe_pg.has_value());
      EXPECT_EQ(maybe_pg->number, "123456");
      EXPECT_EQ(maybe_pg->check_digit, check);
      EXPECT_EQ(maybe_pg->to_string(), valid_pg);
    }

    TEST(PlusgiroParsing, Parse8DigitNumber) {
      logger::scope_logger log_raii{logger::development_trace, "Parse8DigitNumber"};

      char check = account::giro::PG::calculate_check_digit("1234567");
      std::string valid_pg = std::string("1234567-") + check;

      auto maybe_pg = account::giro::PG::to_plusgiro_number(valid_pg);

      ASSERT_TRUE(maybe_pg.has_value());
      EXPECT_EQ(maybe_pg->number, "1234567");
      EXPECT_EQ(maybe_pg->check_digit, check);
      EXPECT_EQ(maybe_pg->to_string(), valid_pg);
    }

    TEST(PlusgiroParsing, ParseWithLeadingWhitespace) {
      logger::scope_logger log_raii{logger::development_trace, "ParseWithLeadingWhitespace"};

      char check = account::giro::PG::calculate_check_digit("1234");
      std::string valid_pg = std::string("1234-") + check;

      auto maybe_pg = account::giro::PG::to_plusgiro_number("  " + valid_pg);

      ASSERT_TRUE(maybe_pg.has_value());
      EXPECT_EQ(maybe_pg->number, "1234");
      EXPECT_EQ(maybe_pg->check_digit, check);
    }

    TEST(PlusgiroParsing, ParseWithTrailingWhitespace) {
      logger::scope_logger log_raii{logger::development_trace, "ParseWithTrailingWhitespace"};

      char check = account::giro::PG::calculate_check_digit("1234");
      std::string valid_pg = std::string("1234-") + check;

      auto maybe_pg = account::giro::PG::to_plusgiro_number(valid_pg + "  ");

      ASSERT_TRUE(maybe_pg.has_value());
      EXPECT_EQ(maybe_pg->number, "1234");
      EXPECT_EQ(maybe_pg->check_digit, check);
    }

    TEST(PlusgiroParsing, ParseWithSurroundingWhitespace) {
      logger::scope_logger log_raii{logger::development_trace, "ParseWithSurroundingWhitespace"};

      char check = account::giro::PG::calculate_check_digit("12345");
      std::string valid_pg = std::string("12345-") + check;

      auto maybe_pg = account::giro::PG::to_plusgiro_number("  " + valid_pg + "  ");

      ASSERT_TRUE(maybe_pg.has_value());
      EXPECT_EQ(maybe_pg->number, "12345");
      EXPECT_EQ(maybe_pg->check_digit, check);
    }

    // ============================================================================
    // Invalid PG number rejection tests - Format errors
    // ============================================================================

    TEST(PlusgiroParsing, RejectNoDash) {
      logger::scope_logger log_raii{logger::development_trace, "RejectNoDash"};

      auto maybe_pg = account::giro::PG::to_plusgiro_number("12345");

      EXPECT_FALSE(maybe_pg.has_value());
    }

    TEST(PlusgiroParsing, RejectMultipleDashes) {
      logger::scope_logger log_raii{logger::development_trace, "RejectMultipleDashes"};

      auto maybe_pg = account::giro::PG::to_plusgiro_number("12-34-5");

      EXPECT_FALSE(maybe_pg.has_value());
    }

    TEST(PlusgiroParsing, RejectWrongDashPosition) {
      logger::scope_logger log_raii{logger::development_trace, "RejectWrongDashPosition"};

      // Dash should be before LAST digit, not in middle
      auto maybe_pg = account::giro::PG::to_plusgiro_number("123-45");

      EXPECT_FALSE(maybe_pg.has_value());
    }

    TEST(PlusgiroParsing, RejectTooFewDigits) {
      logger::scope_logger log_raii{logger::development_trace, "RejectTooFewDigits"};

      auto maybe_pg = account::giro::PG::to_plusgiro_number("123-4");

      EXPECT_FALSE(maybe_pg.has_value());
    }

    TEST(PlusgiroParsing, RejectTooManyDigits) {
      logger::scope_logger log_raii{logger::development_trace, "RejectTooManyDigits"};

      auto maybe_pg = account::giro::PG::to_plusgiro_number("12345678-9");

      EXPECT_FALSE(maybe_pg.has_value());
    }

    TEST(PlusgiroParsing, RejectNonNumericPrefix) {
      logger::scope_logger log_raii{logger::development_trace, "RejectNonNumericPrefix"};

      auto maybe_pg = account::giro::PG::to_plusgiro_number("ABCD-5");

      EXPECT_FALSE(maybe_pg.has_value());
    }

    TEST(PlusgiroParsing, RejectNonNumericCheckDigit) {
      logger::scope_logger log_raii{logger::development_trace, "RejectNonNumericCheckDigit"};

      auto maybe_pg = account::giro::PG::to_plusgiro_number("1234-X");

      EXPECT_FALSE(maybe_pg.has_value());
    }

    TEST(PlusgiroParsing, RejectEmptyString) {
      logger::scope_logger log_raii{logger::development_trace, "RejectEmptyString"};

      auto maybe_pg = account::giro::PG::to_plusgiro_number("");

      EXPECT_FALSE(maybe_pg.has_value());
    }

    TEST(PlusgiroParsing, RejectWhitespaceOnly) {
      logger::scope_logger log_raii{logger::development_trace, "RejectWhitespaceOnly"};

      auto maybe_pg = account::giro::PG::to_plusgiro_number("   ");

      EXPECT_FALSE(maybe_pg.has_value());
    }

    TEST(PlusgiroParsing, RejectDashOnly) {
      logger::scope_logger log_raii{logger::development_trace, "RejectDashOnly"};

      auto maybe_pg = account::giro::PG::to_plusgiro_number("-");

      EXPECT_FALSE(maybe_pg.has_value());
    }

    // ============================================================================
    // Invalid PG number rejection tests - All-zeros
    // ============================================================================

    TEST(PlusgiroParsing, RejectAllZeros5Digits) {
      logger::scope_logger log_raii{logger::development_trace, "RejectAllZeros5Digits"};

      auto maybe_pg = account::giro::PG::to_plusgiro_number("0000-0");

      EXPECT_FALSE(maybe_pg.has_value());
    }

    TEST(PlusgiroParsing, RejectAllZeros6Digits) {
      logger::scope_logger log_raii{logger::development_trace, "RejectAllZeros6Digits"};

      auto maybe_pg = account::giro::PG::to_plusgiro_number("00000-0");

      EXPECT_FALSE(maybe_pg.has_value());
    }

    TEST(PlusgiroParsing, RejectAllZeros7Digits) {
      logger::scope_logger log_raii{logger::development_trace, "RejectAllZeros7Digits"};

      auto maybe_pg = account::giro::PG::to_plusgiro_number("000000-0");

      EXPECT_FALSE(maybe_pg.has_value());
    }

    TEST(PlusgiroParsing, RejectAllZeros8Digits) {
      logger::scope_logger log_raii{logger::development_trace, "RejectAllZeros8Digits"};

      auto maybe_pg = account::giro::PG::to_plusgiro_number("0000000-0");

      EXPECT_FALSE(maybe_pg.has_value());
    }

    // ============================================================================
    // Invalid PG number rejection tests - Check digit validation
    // ============================================================================

    TEST(PlusgiroParsing, RejectInvalidCheckDigit) {
      logger::scope_logger log_raii{logger::development_trace, "RejectInvalidCheckDigit"};

      // Calculate correct check digit, then use wrong one
      char correct_check = account::giro::PG::calculate_check_digit("1234");
      char wrong_check = (correct_check == '9') ? '0' : (correct_check + 1);
      std::string invalid_pg = std::string("1234-") + wrong_check;

      auto maybe_pg = account::giro::PG::to_plusgiro_number(invalid_pg);

      EXPECT_FALSE(maybe_pg.has_value());
    }

    TEST(PlusgiroParsing, RejectWrongCheckDigitAllNines) {
      logger::scope_logger log_raii{logger::development_trace, "RejectWrongCheckDigitAllNines"};

      char correct_check = account::giro::PG::calculate_check_digit("9999");
      char wrong_check = (correct_check == '9') ? '0' : (correct_check + 1);
      std::string invalid_pg = std::string("9999-") + wrong_check;

      auto maybe_pg = account::giro::PG::to_plusgiro_number(invalid_pg);

      EXPECT_FALSE(maybe_pg.has_value());
    }

    TEST(PlusgiroParsing, RejectWrongCheckDigit6Digits) {
      logger::scope_logger log_raii{logger::development_trace, "RejectWrongCheckDigit6Digits"};

      char correct_check = account::giro::PG::calculate_check_digit("12345");
      char wrong_check = (correct_check == '9') ? '0' : (correct_check + 1);
      std::string invalid_pg = std::string("12345-") + wrong_check;

      auto maybe_pg = account::giro::PG::to_plusgiro_number(invalid_pg);

      EXPECT_FALSE(maybe_pg.has_value());
    }

    // ============================================================================
    // Check digit calculation tests
    // ============================================================================

    TEST(PlusgiroCheckDigit, CalculateForSimpleNumber) {
      logger::scope_logger log_raii{logger::development_trace, "CalculateForSimpleNumber"};

      char check = account::giro::PG::calculate_check_digit("1234");

      // Verify it's a digit
      EXPECT_TRUE(check >= '0' && check <= '9');

      // Verify validation works with calculated digit
      EXPECT_TRUE(account::giro::PG::validate_check_digit("1234", check));
    }

    TEST(PlusgiroCheckDigit, CalculateFor4Digits) {
      logger::scope_logger log_raii{logger::development_trace, "CalculateFor4Digits"};

      char check = account::giro::PG::calculate_check_digit("1234");
      EXPECT_TRUE(account::giro::PG::validate_check_digit("1234", check));
    }

    TEST(PlusgiroCheckDigit, CalculateFor5Digits) {
      logger::scope_logger log_raii{logger::development_trace, "CalculateFor5Digits"};

      char check = account::giro::PG::calculate_check_digit("12345");
      EXPECT_TRUE(account::giro::PG::validate_check_digit("12345", check));
    }

    TEST(PlusgiroCheckDigit, CalculateFor6Digits) {
      logger::scope_logger log_raii{logger::development_trace, "CalculateFor6Digits"};

      char check = account::giro::PG::calculate_check_digit("123456");
      EXPECT_TRUE(account::giro::PG::validate_check_digit("123456", check));
    }

    TEST(PlusgiroCheckDigit, CalculateFor7Digits) {
      logger::scope_logger log_raii{logger::development_trace, "CalculateFor7Digits"};

      char check = account::giro::PG::calculate_check_digit("1234567");
      EXPECT_TRUE(account::giro::PG::validate_check_digit("1234567", check));
    }

    TEST(PlusgiroCheckDigit, ValidateCorrectCheckDigit) {
      logger::scope_logger log_raii{logger::development_trace, "ValidateCorrectCheckDigit"};

      char check = account::giro::PG::calculate_check_digit("12345");

      EXPECT_TRUE(account::giro::PG::validate_check_digit("12345", check));
    }

    TEST(PlusgiroCheckDigit, RejectIncorrectCheckDigit) {
      logger::scope_logger log_raii{logger::development_trace, "RejectIncorrectCheckDigit"};

      char correct = account::giro::PG::calculate_check_digit("12345");
      char incorrect = (correct == '9') ? '0' : (correct + 1);

      EXPECT_FALSE(account::giro::PG::validate_check_digit("12345", incorrect));
    }

    TEST(PlusgiroCheckDigit, CalculateDeterministic) {
      logger::scope_logger log_raii{logger::development_trace, "CalculateDeterministic"};

      // Same input should always produce same check digit
      char check1 = account::giro::PG::calculate_check_digit("1234");
      char check2 = account::giro::PG::calculate_check_digit("1234");

      EXPECT_EQ(check1, check2);
    }

    // ============================================================================
    // Canonical format and to_string tests
    // ============================================================================

    TEST(PlusgiroCanonical, FormatPreservesDash) {
      logger::scope_logger log_raii{logger::development_trace, "FormatPreservesDash"};

      char check = account::giro::PG::calculate_check_digit("1234");
      std::string expected = std::string("1234-") + check;

      auto maybe_pg = account::giro::PG::to_plusgiro_number(expected);
      ASSERT_TRUE(maybe_pg.has_value());

      std::string canonical = account::giro::PG::to_canonical_pg(*maybe_pg);

      EXPECT_EQ(canonical, expected);
    }

    TEST(PlusgiroCanonical, FormatTrimsWhitespace) {
      logger::scope_logger log_raii{logger::development_trace, "FormatTrimsWhitespace"};

      char check = account::giro::PG::calculate_check_digit("1234");
      std::string clean = std::string("1234-") + check;
      std::string with_whitespace = "  " + clean + "  ";

      auto maybe_pg = account::giro::PG::to_plusgiro_number(with_whitespace);
      ASSERT_TRUE(maybe_pg.has_value());

      std::string canonical = account::giro::PG::to_canonical_pg(*maybe_pg);

      EXPECT_EQ(canonical, clean);  // Whitespace should be trimmed
    }

    TEST(PlusgiroCanonical, ToStringMatchesCanonical) {
      logger::scope_logger log_raii{logger::development_trace, "ToStringMatchesCanonical"};

      char check = account::giro::PG::calculate_check_digit("12345");
      std::string expected = std::string("12345-") + check;

      auto maybe_pg = account::giro::PG::to_plusgiro_number(expected);
      ASSERT_TRUE(maybe_pg.has_value());

      EXPECT_EQ(maybe_pg->to_string(), account::giro::PG::to_canonical_pg(*maybe_pg));
      EXPECT_EQ(maybe_pg->to_string(), expected);
    }

    TEST(PlusgiroCanonical, ToStringReconstructsOriginal) {
      logger::scope_logger log_raii{logger::development_trace, "ToStringReconstructsOriginal"};

      char check = account::giro::PG::calculate_check_digit("123456");
      std::string original = std::string("123456-") + check;

      auto maybe_pg = account::giro::PG::to_plusgiro_number(original);
      ASSERT_TRUE(maybe_pg.has_value());

      // Parsing and reconstructing should give back the original
      EXPECT_EQ(maybe_pg->to_string(), original);
    }

    TEST(PlusgiroStruct, ComponentsCorrectlySeparated) {
      logger::scope_logger log_raii{logger::development_trace, "ComponentsCorrectlySeparated"};

      char check = account::giro::PG::calculate_check_digit("1234567");
      std::string input = std::string("1234567-") + check;

      auto maybe_pg = account::giro::PG::to_plusgiro_number(input);
      ASSERT_TRUE(maybe_pg.has_value());

      EXPECT_EQ(maybe_pg->number, "1234567");
      EXPECT_EQ(maybe_pg->check_digit, check);
      EXPECT_EQ(maybe_pg->number.size(), 7u);
    }

  } // namespace pg

} // namespace tests::giro_framework
