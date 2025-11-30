#include "test_atomics.hpp"
#include "test_fixtures.hpp"
#include "logger/log.hpp"
#include "fiscal/amount/AccountStatement.hpp"
#include "fiscal/amount/TaggedAmountFramework.hpp"
#include "fiscal/amount/AmountFramework.hpp"
#include "domain/account_statement_to_tagged_amounts.hpp"
#include "domain/csv_to_account_statement.hpp"
#include "csv/parse_csv.hpp"
#include "test/data/account_statements_csv.hpp"
#include <gtest/gtest.h>
#include <optional>
#include <string>

namespace tests::statement_to_tagged_amounts {

/**
 * Test Suite: AccountStatement -> TaggedAmounts Transformation (Step 8)
 *
 * This test suite verifies the transformation of AccountStatement to TaggedAmounts
 * as part of the CSV import pipeline refactoring.
 *
 * Tests cover:
 * - Single entry transformation
 * - Multiple entries transformation
 * - Empty statement handling
 * - Missing AccountID handling
 * - Negative amounts
 * - Amount conversion (Amount -> CentsAmount)
 * - Tag correctness ("Text" and "Account")
 * - Integration with real CSV data
 */

// Test fixture for AccountStatement -> TaggedAmounts tests
class StatementToTaggedAmountsTestFixture : public ::testing::Test {
protected:
  // Helper to create a simple AccountStatementEntry
  static AccountStatementEntry make_entry(
      int year, unsigned month, unsigned day,
      double amount,
      std::string const& caption) {
    Date date{std::chrono::year{year}, std::chrono::month{month}, std::chrono::day{day}};
    return AccountStatementEntry(date, Amount{amount}, caption);
  }

  // Helper to create an AccountID
  static AccountID make_account_id(std::string const& prefix, std::string const& value) {
    return AccountID{.m_prefix = prefix, .m_value = value};
  }
};

// ============================================================================
// Single Entry Tests
// ============================================================================

TEST_F(StatementToTaggedAmountsTestFixture, SingleEntryTransformation) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(StatementToTaggedAmounts, SingleEntryTransformation)"};

  // Create a single entry
  AccountStatementEntries entries;
  entries.push_back(make_entry(2025, 1, 15, 100.50, "Test Transaction"));

  // Create AccountStatement with AccountID
  AccountID account_id = make_account_id("NORDEA", "51 86 87-9");
  AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};
  AccountStatement statement(entries, meta);

  // Transform to TaggedAmounts
  auto result = domain::account_statement_to_tagged_amounts_step(statement);

  // Verify result
  ASSERT_TRUE(result.has_value()) << "Expected successful transformation";
  ASSERT_EQ(result->size(), 1) << "Expected exactly one TaggedAmount";

  auto const& ta = (*result)[0];

  // Verify date
  EXPECT_EQ(ta.date().year(), std::chrono::year{2025});
  EXPECT_EQ(ta.date().month(), std::chrono::month{1});
  EXPECT_EQ(ta.date().day(), std::chrono::day{15});

  // Verify amount (100.50 -> 10050 cents)
  EXPECT_EQ(to_amount_in_cents_integer(ta.cents_amount()), 10050)
    << "Expected 100.50 to convert to 10050 cents";

  // Verify tags
  ASSERT_TRUE(ta.tags().contains("Text")) << "Expected 'Text' tag";
  EXPECT_EQ(ta.tags().at("Text"), "Test Transaction");

  ASSERT_TRUE(ta.tags().contains("Account")) << "Expected 'Account' tag";
  EXPECT_EQ(ta.tags().at("Account"), "NORDEA::51 86 87-9");
}

// ============================================================================
// Multiple Entries Tests
// ============================================================================

TEST_F(StatementToTaggedAmountsTestFixture, MultipleEntriesTransformation) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(StatementToTaggedAmounts, MultipleEntriesTransformation)"};

  // Create multiple entries
  AccountStatementEntries entries;
  entries.push_back(make_entry(2025, 1, 10, 500.00, "First Transaction"));
  entries.push_back(make_entry(2025, 1, 15, -250.75, "Second Transaction"));
  entries.push_back(make_entry(2025, 1, 20, 1000.00, "Third Transaction"));

  // Create AccountStatement
  AccountID account_id = make_account_id("SKV", "5567828172");
  AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};
  AccountStatement statement(entries, meta);

  // Transform
  auto result = domain::account_statement_to_tagged_amounts_step(statement);

  // Verify
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 3) << "Expected three TaggedAmounts";

  // All should have the same Account tag
  for (auto const& ta : *result) {
    ASSERT_TRUE(ta.tags().contains("Account"));
    EXPECT_EQ(ta.tags().at("Account"), "SKV::5567828172");
  }

  // Verify individual entries
  EXPECT_EQ((*result)[0].tags().at("Text"), "First Transaction");
  EXPECT_EQ(to_amount_in_cents_integer((*result)[0].cents_amount()), 50000);

  EXPECT_EQ((*result)[1].tags().at("Text"), "Second Transaction");
  EXPECT_EQ(to_amount_in_cents_integer((*result)[1].cents_amount()), -25075);

  EXPECT_EQ((*result)[2].tags().at("Text"), "Third Transaction");
  EXPECT_EQ(to_amount_in_cents_integer((*result)[2].cents_amount()), 100000);
}

// ============================================================================
// Empty Statement Tests
// ============================================================================

TEST_F(StatementToTaggedAmountsTestFixture, EmptyStatementReturnsEmptyVector) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(StatementToTaggedAmounts, EmptyStatementReturnsEmptyVector)"};

  // Create empty entries
  AccountStatementEntries entries;

  // Create AccountStatement with AccountID
  AccountID account_id = make_account_id("NORDEA", "12 34 56-7");
  AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};
  AccountStatement statement(entries, meta);

  // Transform
  auto result = domain::account_statement_to_tagged_amounts_step(statement);

  // Empty statement should return empty vector, NOT nullopt
  ASSERT_TRUE(result.has_value()) << "Expected valid result for empty statement";
  EXPECT_TRUE(result->empty()) << "Expected empty vector for empty statement";
}

// ============================================================================
// Missing AccountID Tests
// ============================================================================

TEST_F(StatementToTaggedAmountsTestFixture, MissingAccountIdUsesEmptyString) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(StatementToTaggedAmounts, MissingAccountIdUsesEmptyString)"};

  // Create entry
  AccountStatementEntries entries;
  entries.push_back(make_entry(2025, 2, 1, 75.25, "Transaction without AccountID"));

  // Create AccountStatement WITHOUT AccountID
  AccountStatement::Meta meta{.m_maybe_account_irl_id = std::nullopt};
  AccountStatement statement(entries, meta);

  // Transform
  auto result = domain::account_statement_to_tagged_amounts_step(statement);

  // Verify
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 1);

  auto const& ta = (*result)[0];

  // Text tag should be present
  ASSERT_TRUE(ta.tags().contains("Text"));
  EXPECT_EQ(ta.tags().at("Text"), "Transaction without AccountID");

  // Account tag should be empty string (graceful handling)
  ASSERT_TRUE(ta.tags().contains("Account"));
  EXPECT_EQ(ta.tags().at("Account"), "") << "Expected empty string for missing AccountID";
}

// ============================================================================
// Negative Amount Tests
// ============================================================================

TEST_F(StatementToTaggedAmountsTestFixture, NegativeAmountsConvertedCorrectly) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(StatementToTaggedAmounts, NegativeAmountsConvertedCorrectly)"};

  // Create entries with negative amounts
  AccountStatementEntries entries;
  entries.push_back(make_entry(2025, 3, 1, -1083.75, "Payment Out"));
  entries.push_back(make_entry(2025, 3, 2, -0.01, "Minimal Negative"));
  entries.push_back(make_entry(2025, 3, 3, -9999.99, "Large Negative"));

  AccountID account_id = make_account_id("NORDEA", "51 86 87-9");
  AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};
  AccountStatement statement(entries, meta);

  auto result = domain::account_statement_to_tagged_amounts_step(statement);

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 3);

  // -1083.75 -> -108375 cents
  EXPECT_EQ(to_amount_in_cents_integer((*result)[0].cents_amount()), -108375);

  // -0.01 -> -1 cent
  EXPECT_EQ(to_amount_in_cents_integer((*result)[1].cents_amount()), -1);

  // -9999.99 -> -999999 cents
  EXPECT_EQ(to_amount_in_cents_integer((*result)[2].cents_amount()), -999999);
}

// ============================================================================
// Amount Conversion Tests
// ============================================================================

TEST_F(StatementToTaggedAmountsTestFixture, AmountConversionEdgeCases) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(StatementToTaggedAmounts, AmountConversionEdgeCases)"};

  AccountStatementEntries entries;

  // Zero amount
  entries.push_back(make_entry(2025, 4, 1, 0.0, "Zero Amount"));

  // Fractional cents (should round)
  entries.push_back(make_entry(2025, 4, 2, 10.005, "Fractional Cents"));

  // Large amount
  entries.push_back(make_entry(2025, 4, 3, 1000000.00, "One Million"));

  AccountID account_id = make_account_id("TEST:", "123");
  AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};
  AccountStatement statement(entries, meta);

  auto result = domain::account_statement_to_tagged_amounts_step(statement);

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 3);

  // Zero -> 0 cents
  EXPECT_EQ(to_amount_in_cents_integer((*result)[0].cents_amount()), 0);

  // 10.005 should round to 1001 cents (std::round behavior)
  EXPECT_EQ(to_amount_in_cents_integer((*result)[1].cents_amount()), 1001);

  // 1,000,000.00 -> 100,000,000 cents
  EXPECT_EQ(to_amount_in_cents_integer((*result)[2].cents_amount()), 100000000);
}

// ============================================================================
// Date Preservation Tests
// ============================================================================

TEST_F(StatementToTaggedAmountsTestFixture, DatePreservedCorrectly) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(StatementToTaggedAmounts, DatePreservedCorrectly)"};

  AccountStatementEntries entries;
  entries.push_back(make_entry(2025, 12, 31, 100.0, "End of Year"));
  entries.push_back(make_entry(2025, 1, 1, 200.0, "Start of Year"));
  entries.push_back(make_entry(2025, 6, 15, 300.0, "Mid Year"));

  AccountID account_id = make_account_id("TEST:", "456");
  AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};
  AccountStatement statement(entries, meta);

  auto result = domain::account_statement_to_tagged_amounts_step(statement);

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 3);

  // Entry 0: 2025-12-31
  EXPECT_EQ((*result)[0].date().year(), std::chrono::year{2025});
  EXPECT_EQ((*result)[0].date().month(), std::chrono::month{12});
  EXPECT_EQ((*result)[0].date().day(), std::chrono::day{31});

  // Entry 1: 2025-01-01
  EXPECT_EQ((*result)[1].date().year(), std::chrono::year{2025});
  EXPECT_EQ((*result)[1].date().month(), std::chrono::month{1});
  EXPECT_EQ((*result)[1].date().day(), std::chrono::day{1});

  // Entry 2: 2025-06-15
  EXPECT_EQ((*result)[2].date().year(), std::chrono::year{2025});
  EXPECT_EQ((*result)[2].date().month(), std::chrono::month{6});
  EXPECT_EQ((*result)[2].date().day(), std::chrono::day{15});
}

// ============================================================================
// Integration Tests with Real CSV Data
// ============================================================================

TEST_F(StatementToTaggedAmountsTestFixture, IntegrationWithNordeaCsvData) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(StatementToTaggedAmounts, IntegrationWithNordeaCsvData)"};

  // Parse NORDEA CSV data
  std::string csv_data = sz_NORDEA_csv_20251120;
  auto maybe_table = CSV::parse::maybe::csv_to_table(csv_data, ';');

  ASSERT_TRUE(maybe_table.has_value()) << "Expected successful CSV parsing";

  // Create AccountID for NORDEA
  AccountID account_id = domain::make_account_id("NORDEA", "51 86 87-9");

  // Step 7: CSV::Table -> AccountStatement
  auto maybe_statement = account::statement::csv_table_to_account_statement_step(*maybe_table, account_id);

  ASSERT_TRUE(maybe_statement.has_value()) << "Expected successful statement creation";

  // Step 8: AccountStatement -> TaggedAmounts
  auto result = domain::account_statement_to_tagged_amounts_step(*maybe_statement);

  ASSERT_TRUE(result.has_value()) << "Expected successful transformation";
  EXPECT_GT(result->size(), 0) << "Expected at least one TaggedAmount";

  // Verify all entries have correct Account tag
  for (auto const& ta : *result) {
    ASSERT_TRUE(ta.tags().contains("Account"));
    EXPECT_EQ(ta.tags().at("Account"), "NORDEA::51 86 87-9");

    ASSERT_TRUE(ta.tags().contains("Text"));
    EXPECT_FALSE(ta.tags().at("Text").empty()) << "Expected non-empty Text tag";
  }

  // Verify first entry (2025/09/29, -1083.75, LOOPIA)
  if (result->size() > 0) {
    auto const& first = (*result)[0];
    EXPECT_EQ(to_amount_in_cents_integer(first.cents_amount()), -108375);
    EXPECT_TRUE(first.tags().at("Text").find("LOOPIA") != std::string::npos ||
                first.tags().at("Text").find("Webhotell") != std::string::npos);
  }
}

TEST_F(StatementToTaggedAmountsTestFixture, IntegrationWithSkvCsvData) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(StatementToTaggedAmounts, IntegrationWithSkvCsvData)"};

  // Parse SKV CSV data
  std::string csv_data = sz_SKV_csv_older;
  auto maybe_table = CSV::parse::maybe::csv_to_table(csv_data, ';');

  ASSERT_TRUE(maybe_table.has_value()) << "Expected successful CSV parsing";

  // Create AccountID for SKV
  AccountID account_id = domain::make_account_id("SKV", "5567828172");

  // Step 7: CSV::Table -> AccountStatement
  auto maybe_statement = account::statement::csv_table_to_account_statement_step(*maybe_table, account_id);

  ASSERT_TRUE(maybe_statement.has_value()) << "Expected successful statement creation";

  // Step 8: AccountStatement -> TaggedAmounts
  auto result = domain::account_statement_to_tagged_amounts_step(*maybe_statement);

  ASSERT_TRUE(result.has_value()) << "Expected successful transformation";

  // Verify all entries have correct Account tag
  for (auto const& ta : *result) {
    ASSERT_TRUE(ta.tags().contains("Account"));
    EXPECT_EQ(ta.tags().at("Account"), "SKV::5567828172");
  }
}

// ============================================================================
// Composed Function Test
// ============================================================================

TEST_F(StatementToTaggedAmountsTestFixture, ComposedCsvTableToTaggedAmounts) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(StatementToTaggedAmounts, ComposedCsvTableToTaggedAmounts)"};

  // Parse NORDEA CSV data
  std::string csv_data = sz_NORDEA_csv_20251120;
  auto maybe_table = CSV::parse::maybe::csv_to_table(csv_data, ';');

  ASSERT_TRUE(maybe_table.has_value()) << "Expected successful CSV parsing";

  // Create AccountID for NORDEA
  AccountID account_id = domain::make_account_id("NORDEA", "51 86 87-9");

  // Use composed function (Steps 7+8 in one call)
  auto result = domain::csv_table_to_tagged_amounts_shortcut(*maybe_table, account_id);

  ASSERT_TRUE(result.has_value()) << "Expected successful composed transformation";
  EXPECT_GT(result->size(), 0) << "Expected at least one TaggedAmount";

  // Verify tags are correctly set
  for (auto const& ta : *result) {
    ASSERT_TRUE(ta.tags().contains("Account"));
    ASSERT_TRUE(ta.tags().contains("Text"));
    EXPECT_EQ(ta.tags().at("Account"), "NORDEA::51 86 87-9");
  }
}

// ============================================================================
// AccountID Format Tests
// ============================================================================

TEST_F(StatementToTaggedAmountsTestFixture, AccountIdWithEmptyPrefix) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(StatementToTaggedAmounts, AccountIdWithEmptyPrefix)"};

  AccountStatementEntries entries;
  entries.push_back(make_entry(2025, 5, 1, 50.0, "Test"));

  // AccountID with empty prefix
  AccountID account_id = make_account_id("", "123456789");
  AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};
  AccountStatement statement(entries, meta);

  auto result = domain::account_statement_to_tagged_amounts_step(statement);

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 1);

  // Account tag should just be the value without prefix
  EXPECT_EQ((*result)[0].tags().at("Account"), "123456789");
}

TEST_F(StatementToTaggedAmountsTestFixture, AccountIdWithVariousPrefixes) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(StatementToTaggedAmounts, AccountIdWithVariousPrefixes)"};

  std::vector<std::pair<std::string, std::string>> test_cases = {
    {"NORDEA", "51 86 87-9"},
    {"SKV", "5567828172"},
    {"PG", "90 00 00-1"},
    {"BG", "123-4567"},
    {"IBAN", "SE35 5000 0000 0549 1000 0003"}
  };

  for (auto const& [prefix, value] : test_cases) {
    AccountStatementEntries entries;
    entries.push_back(make_entry(2025, 1, 1, 100.0, "Test"));

    AccountID account_id = make_account_id(prefix, value);
    AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};
    AccountStatement statement(entries, meta);

    auto result = domain::account_statement_to_tagged_amounts_step(statement);

    ASSERT_TRUE(result.has_value()) << "Failed for prefix: " << prefix;
    ASSERT_EQ(result->size(), 1);

    std::string expected_tag = prefix.empty() ? value : prefix + "::" + value;
    EXPECT_EQ((*result)[0].tags().at("Account"), expected_tag)
      << "Account tag mismatch for prefix: " << prefix;
  }
}

} // namespace tests::statement_to_tagged_amounts
