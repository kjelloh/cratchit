#include "fiscal/BBANFramework.hpp"
#include "logger/log.hpp"
#include <gtest/gtest.h>

namespace tests::swedish_bank_account {

  // Test suite for basic parsing functionality
  namespace parsing_suite {

    TEST(SwedishBankAccountParsing, ParseNordeaAccountWithDash) {
      logger::scope_logger log_raii{logger::development_trace, "ParseNordeaAccountWithDash"};

      auto maybe_account = account::BBAN::to_swedish_bank_account("3000-1234567");

      ASSERT_TRUE(maybe_account.has_value());
      EXPECT_EQ(maybe_account->clearing_number, "3000");
      EXPECT_EQ(maybe_account->account_number, "1234567");
      EXPECT_EQ(maybe_account->bank_name, "Nordea");
      EXPECT_EQ(maybe_account->type, account::BBAN::AccountType::Type1Variant1);
    }

    TEST(SwedishBankAccountParsing, ParseNordeaAccountWithoutDash) {
      logger::scope_logger log_raii{logger::development_trace, "ParseNordeaAccountWithoutDash"};

      auto maybe_account = account::BBAN::to_swedish_bank_account("30001234567");

      ASSERT_TRUE(maybe_account.has_value());
      EXPECT_EQ(maybe_account->clearing_number, "3000");
      EXPECT_EQ(maybe_account->account_number, "1234567");
      EXPECT_EQ(maybe_account->bank_name, "Nordea");
    }

    TEST(SwedishBankAccountParsing, ParseSEBAccount) {
      logger::scope_logger log_raii{logger::development_trace, "ParseSEBAccount"};

      auto maybe_account = account::BBAN::to_swedish_bank_account("5000-9876543");

      ASSERT_TRUE(maybe_account.has_value());
      EXPECT_EQ(maybe_account->clearing_number, "5000");
      EXPECT_EQ(maybe_account->account_number, "9876543");
      EXPECT_EQ(maybe_account->bank_name, "SEB");
      EXPECT_EQ(maybe_account->type, account::BBAN::AccountType::Type1Variant1);
    }

    TEST(SwedishBankAccountParsing, ParseSwedbank) {
      logger::scope_logger log_raii{logger::development_trace, "ParseSwedbank"};

      auto maybe_account = account::BBAN::to_swedish_bank_account("7000-1111111");

      ASSERT_TRUE(maybe_account.has_value());
      EXPECT_EQ(maybe_account->clearing_number, "7000");
      EXPECT_EQ(maybe_account->account_number, "1111111");
      EXPECT_EQ(maybe_account->bank_name, "Swedbank");
      EXPECT_EQ(maybe_account->type, account::BBAN::AccountType::Type1Variant1);
    }

    TEST(SwedishBankAccountParsing, ParseHandelsbanken) {
      logger::scope_logger log_raii{logger::development_trace, "ParseHandelsbanken"};

      auto maybe_account = account::BBAN::to_swedish_bank_account("6000-123456789");

      ASSERT_TRUE(maybe_account.has_value());
      EXPECT_EQ(maybe_account->clearing_number, "6000");
      EXPECT_EQ(maybe_account->account_number, "123456789");
      EXPECT_EQ(maybe_account->bank_name, "Handelsbanken");
      EXPECT_EQ(maybe_account->type, account::BBAN::AccountType::Type1Variant2);
    }

    TEST(SwedishBankAccountParsing, ParseWithWhitespace) {
      logger::scope_logger log_raii{logger::development_trace, "ParseWithWhitespace"};

      auto maybe_account = account::BBAN::to_swedish_bank_account("  3000 1234567  ");

      ASSERT_TRUE(maybe_account.has_value());
      EXPECT_EQ(maybe_account->clearing_number, "3000");
      EXPECT_EQ(maybe_account->account_number, "1234567");
      EXPECT_EQ(maybe_account->bank_name, "Nordea");
    }

    TEST(SwedishBankAccountParsing, ParseWithMultipleSeparators) {
      logger::scope_logger log_raii{logger::development_trace, "ParseWithMultipleSeparators"};

      auto maybe_account = account::BBAN::to_swedish_bank_account("3000 - 1234567");

      ASSERT_TRUE(maybe_account.has_value());
      EXPECT_EQ(maybe_account->clearing_number, "3000");
      EXPECT_EQ(maybe_account->account_number, "1234567");
    }

    TEST(SwedishBankAccountParsing, RejectEmptyString) {
      logger::scope_logger log_raii{logger::development_trace, "RejectEmptyString"};

      auto maybe_account = account::BBAN::to_swedish_bank_account("");

      EXPECT_FALSE(maybe_account.has_value());
    }

    TEST(SwedishBankAccountParsing, RejectWhitespaceOnly) {
      logger::scope_logger log_raii{logger::development_trace, "RejectWhitespaceOnly"};

      auto maybe_account = account::BBAN::to_swedish_bank_account("   ");

      EXPECT_FALSE(maybe_account.has_value());
    }

    TEST(SwedishBankAccountParsing, RejectTooShort) {
      logger::scope_logger log_raii{logger::development_trace, "RejectTooShort"};

      auto maybe_account = account::BBAN::to_swedish_bank_account("3000");

      EXPECT_FALSE(maybe_account.has_value());
    }

    TEST(SwedishBankAccountParsing, RejectUnknownBank) {
      logger::scope_logger log_raii{logger::development_trace, "RejectUnknownBank"};

      // Clearing number 9999 not in registry
      auto maybe_account = account::BBAN::to_swedish_bank_account("9999-1234567");

      EXPECT_FALSE(maybe_account.has_value());
    }

    TEST(SwedishBankAccountParsing, RejectNonNumeric) {
      logger::scope_logger log_raii{logger::development_trace, "RejectNonNumeric"};

      auto maybe_account = account::BBAN::to_swedish_bank_account("ABC-1234567");

      EXPECT_FALSE(maybe_account.has_value());
    }

  } // namespace parsing_suite

  // Test suite for canonical format conversion
  namespace canonical_format_suite {

    TEST(SwedishBankAccountCanonical, FormatNordeaAccount) {
      logger::scope_logger log_raii{logger::development_trace, "FormatNordeaAccount"};

      auto maybe_account = account::BBAN::to_swedish_bank_account("30001234567");
      ASSERT_TRUE(maybe_account.has_value());

      std::string canonical = account::BBAN::to_canonical_bban(*maybe_account);

      EXPECT_EQ(canonical, "3000-1234567");
    }

    TEST(SwedishBankAccountCanonical, FormatPreservesStructure) {
      logger::scope_logger log_raii{logger::development_trace, "FormatPreservesStructure"};

      auto maybe_account = account::BBAN::to_swedish_bank_account("  5000  -  9876543  ");
      ASSERT_TRUE(maybe_account.has_value());

      std::string canonical = account::BBAN::to_canonical_bban(*maybe_account);

      EXPECT_EQ(canonical, "5000-9876543");
    }

  } // namespace canonical_format_suite

  // Test suite for bank name lookup
  namespace bank_name_suite {

    TEST(SwedishBankAccountBankName, LookupNordea) {
      logger::scope_logger log_raii{logger::development_trace, "LookupNordea"};

      auto maybe_name = account::BBAN::to_bank_name("3000");

      ASSERT_TRUE(maybe_name.has_value());
      EXPECT_EQ(*maybe_name, "Nordea");
    }

    TEST(SwedishBankAccountBankName, LookupSEB) {
      logger::scope_logger log_raii{logger::development_trace, "LookupSEB"};

      auto maybe_name = account::BBAN::to_bank_name("5000");

      ASSERT_TRUE(maybe_name.has_value());
      EXPECT_EQ(*maybe_name, "SEB");
    }

    TEST(SwedishBankAccountBankName, LookupSwedbank) {
      logger::scope_logger log_raii{logger::development_trace, "LookupSwedbank"};

      auto maybe_name = account::BBAN::to_bank_name("7000");

      ASSERT_TRUE(maybe_name.has_value());
      EXPECT_EQ(*maybe_name, "Swedbank");
    }

    TEST(SwedishBankAccountBankName, RejectUnknownClearing) {
      logger::scope_logger log_raii{logger::development_trace, "RejectUnknownClearing"};

      auto maybe_name = account::BBAN::to_bank_name("9999");

      EXPECT_FALSE(maybe_name.has_value());
    }

  } // namespace bank_name_suite

  // Test suite for account type classification
  namespace account_type_suite {

    TEST(SwedishBankAccountType, ClassifyNordeaAsType1Variant1) {
      logger::scope_logger log_raii{logger::development_trace, "ClassifyNordeaAsType1Variant1"};

      auto type = account::BBAN::to_account_type("3000");

      EXPECT_EQ(type, account::BBAN::AccountType::Type1Variant1);
    }

    TEST(SwedishBankAccountType, ClassifyHandelsbankenAsType1Variant2) {
      logger::scope_logger log_raii{logger::development_trace, "ClassifyHandelsbankenAsType1Variant2"};

      auto type = account::BBAN::to_account_type("6000");

      EXPECT_EQ(type, account::BBAN::AccountType::Type1Variant2);
    }

    TEST(SwedishBankAccountType, ClassifySwedbank8xxxAsType2) {
      logger::scope_logger log_raii{logger::development_trace, "ClassifySwedbank8xxxAsType2"};

      auto type = account::BBAN::to_account_type("8000");

      EXPECT_EQ(type, account::BBAN::AccountType::Type2);
    }

    TEST(SwedishBankAccountType, ReturnUnknownForInvalidClearing) {
      logger::scope_logger log_raii{logger::development_trace, "ReturnUnknownForInvalidClearing"};

      auto type = account::BBAN::to_account_type("9999");

      EXPECT_EQ(type, account::BBAN::AccountType::Unknown);
    }

    TEST(SwedishBankAccountType, ReturnUndefinedForNonNumeric) {
      logger::scope_logger log_raii{logger::development_trace, "ReturnUndefinedForNonNumeric"};

      auto type = account::BBAN::to_account_type("ABC");

      EXPECT_EQ(type, account::BBAN::AccountType::Undefined);
    }

  } // namespace account_type_suite

} // namespace tests::swedish_bank_account
