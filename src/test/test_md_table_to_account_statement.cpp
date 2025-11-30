#include "test_atomics.hpp"
#include "test_fixtures.hpp"
#include "logger/log.hpp"
#include "fiscal/amount/AccountStatement.hpp"
#include "domain/csv_to_account_statement.hpp"
#include "csv/csv_to_account_id.hpp"
#include "csv/parse_csv.hpp"
#include "test/data/account_statements_csv.hpp"
#include <gtest/gtest.h>
#include <optional>
#include <string>

namespace tests::md_table_to_account_statement {

/**
 * Test Suite: MDTable<AccountID> -> AccountStatement Transformation (Step 7 explicit)
 *
 * This test suite verifies the explicit transformation from CSV::MDTable<AccountID>
 * to AccountStatement. These tests focus ONLY on the aggregation behavior:
 * - MDTable structure unpacking
 * - AccountID propagation to AccountStatement metadata
 *
 * Entry extraction logic is tested separately in csv_table_to_account_statement_step_entries tests.
 */

// Test fixture for MDTable -> AccountStatement tests
class MDTableToAccountStatementTestFixture : public ::testing::Test {
protected:
  // Helper to create an MDTable from CSV text using the real parsing/detection pipeline
  static std::optional<CSV::MDTable<AccountID>> make_md_table_from_csv(
      std::string const& csv_text,
      char delimiter = ';') {
    auto maybe_table = CSV::parse::maybe::csv_to_table(csv_text, delimiter);
    if (!maybe_table) {
      return std::nullopt;
    }
    return account::statement::maybe::to_account_id_ed_step(*maybe_table);
  }

  // Helper to create an MDTable directly with specific AccountID and Table
  static CSV::MDTable<AccountID> make_md_table(
      AccountID const& account_id,
      CSV::Table const& table) {
    return CSV::MDTable<AccountID>{account_id, table};
  }

  // Helper to create a minimal CSV::Table with valid structure
  static CSV::Table make_minimal_table() {
    CSV::Table table;
    table.heading = Key::Path(std::vector<std::string>{"Datum", "Belopp", "Namn"});
    table.rows = {
      Key::Path(std::vector<std::string>{"Datum", "Belopp", "Namn"}),  // Header row (duplicated as per CSV::Table convention)
      Key::Path(std::vector<std::string>{"2025-01-15", "100,50", "Test Transaction"})
    };
    return table;
  }
};

// ============================================================================
// MDTable Unpacking Tests - Verify AccountID is correctly propagated
// ============================================================================

TEST_F(MDTableToAccountStatementTestFixture, AccountIdPropagatedToStatementMeta) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(MDTableToAccountStatement, AccountIdPropagatedToStatementMeta)"};

  // Create MDTable with specific AccountID
  AccountID account_id{"NORDEA", "51 86 87-9"};
  CSV::Table table = make_minimal_table();
  auto md_table = make_md_table(account_id, table);

  // Transform
  auto result = account::statement::md_table_to_account_statement(md_table);

  // Verify AccountID is correctly propagated
  ASSERT_TRUE(result.has_value()) << "Expected successful transformation";
  ASSERT_TRUE(result->meta().m_maybe_account_irl_id.has_value())
    << "Expected AccountID to be set in meta";
  EXPECT_EQ(result->meta().m_maybe_account_irl_id->m_prefix, "NORDEA");
  EXPECT_EQ(result->meta().m_maybe_account_irl_id->m_value, "51 86 87-9");
  EXPECT_EQ(result->meta().m_maybe_account_irl_id->to_string(), "NORDEA::51 86 87-9");
}

TEST_F(MDTableToAccountStatementTestFixture, SKVAccountIdPropagatedCorrectly) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(MDTableToAccountStatement, SKVAccountIdPropagatedCorrectly)"};

  // Create MDTable with SKV AccountID
  AccountID account_id{"SKV", "5567828172"};
  CSV::Table table = make_minimal_table();
  auto md_table = make_md_table(account_id, table);

  // Transform
  auto result = account::statement::md_table_to_account_statement(md_table);

  // Verify AccountID
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->meta().m_maybe_account_irl_id.has_value());
  EXPECT_EQ(result->meta().m_maybe_account_irl_id->m_prefix, "SKV");
  EXPECT_EQ(result->meta().m_maybe_account_irl_id->m_value, "5567828172");
}

TEST_F(MDTableToAccountStatementTestFixture, EmptyPrefixAccountIdHandled) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(MDTableToAccountStatement, EmptyPrefixAccountIdHandled)"};

  // Create MDTable with empty prefix AccountID
  AccountID account_id{"", "123456789"};
  CSV::Table table = make_minimal_table();
  auto md_table = make_md_table(account_id, table);

  // Transform
  auto result = account::statement::md_table_to_account_statement(md_table);

  // Verify AccountID with empty prefix
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->meta().m_maybe_account_irl_id.has_value());
  EXPECT_EQ(result->meta().m_maybe_account_irl_id->m_prefix, "");
  EXPECT_EQ(result->meta().m_maybe_account_irl_id->m_value, "123456789");
  EXPECT_EQ(result->meta().m_maybe_account_irl_id->to_string(), "123456789");
}

// ============================================================================
// Integration Tests - Using Real CSV Data Through Full Detection Pipeline
// ============================================================================

TEST_F(MDTableToAccountStatementTestFixture, NordeaMDTableToAccountStatement) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(MDTableToAccountStatement, NordeaMDTableToAccountStatement)"};

  // Parse NORDEA CSV and detect account
  auto maybe_md_table = make_md_table_from_csv(sz_NORDEA_csv_20251120);

  ASSERT_TRUE(maybe_md_table.has_value()) << "Expected successful MDTable creation";

  // Verify detected AccountID before transformation
  EXPECT_EQ(maybe_md_table->meta.m_prefix, "NORDEA");
  EXPECT_FALSE(maybe_md_table->meta.m_value.empty()) << "Expected account number to be detected";

  // Transform MDTable to AccountStatement
  auto result = account::statement::md_table_to_account_statement(*maybe_md_table);

  ASSERT_TRUE(result.has_value()) << "Expected successful transformation";

  // Verify AccountID propagated correctly
  ASSERT_TRUE(result->meta().m_maybe_account_irl_id.has_value());
  EXPECT_EQ(result->meta().m_maybe_account_irl_id->m_prefix, "NORDEA");

  // Verify entries were created
  EXPECT_GT(result->entries().size(), 0) << "Expected at least one entry";
}

TEST_F(MDTableToAccountStatementTestFixture, SKVOlderMDTableToAccountStatement) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(MDTableToAccountStatement, SKVOlderMDTableToAccountStatement)"};

  // Parse SKV CSV (older format) and detect account
  auto maybe_md_table = make_md_table_from_csv(sz_SKV_csv_older);

  ASSERT_TRUE(maybe_md_table.has_value()) << "Expected successful MDTable creation";

  // Verify detected AccountID
  EXPECT_EQ(maybe_md_table->meta.m_prefix, "SKV");

  // Transform MDTable to AccountStatement
  auto result = account::statement::md_table_to_account_statement(*maybe_md_table);

  ASSERT_TRUE(result.has_value()) << "Expected successful transformation";

  // Verify AccountID propagated
  ASSERT_TRUE(result->meta().m_maybe_account_irl_id.has_value());
  EXPECT_EQ(result->meta().m_maybe_account_irl_id->m_prefix, "SKV");
}

TEST_F(MDTableToAccountStatementTestFixture, SKVNewerMDTableToAccountStatement) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(MDTableToAccountStatement, SKVNewerMDTableToAccountStatement)"};

  // Parse SKV CSV (newer format with company name) and detect account
  // Note: newer format uses comma delimiter in quotes
  auto maybe_table = CSV::parse::maybe::csv_to_table(sz_SKV_csv_20251120, ';');
  ASSERT_TRUE(maybe_table.has_value()) << "Expected successful CSV parsing";

  auto maybe_md_table = account::statement::maybe::to_account_id_ed_step(*maybe_table);
  ASSERT_TRUE(maybe_md_table.has_value()) << "Expected successful MDTable creation";

  // Verify detected AccountID (should find org number 5567828172)
  EXPECT_EQ(maybe_md_table->meta.m_prefix, "SKV");

  // Transform MDTable to AccountStatement
  auto result = account::statement::md_table_to_account_statement(*maybe_md_table);

  ASSERT_TRUE(result.has_value()) << "Expected successful transformation";

  // Verify AccountID propagated
  ASSERT_TRUE(result->meta().m_maybe_account_irl_id.has_value());
  EXPECT_EQ(result->meta().m_maybe_account_irl_id->m_prefix, "SKV");
}

// ============================================================================
// AccountID Variation Tests
// ============================================================================

TEST_F(MDTableToAccountStatementTestFixture, VariousAccountIdPrefixesPropagated) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(MDTableToAccountStatement, VariousAccountIdPrefixesPropagated)"};

  std::vector<std::pair<std::string, std::string>> test_cases = {
    {"NORDEA", "51 86 87-9"},
    {"SKV", "5567828172"},
    {"PG", "90 00 00-1"},
    {"BG", "123-4567"},
    {"IBAN", "SE35 5000 0000 0549 1000 0003"},
    {"", "plain-account-number"}
  };

  for (auto const& [prefix, value] : test_cases) {
    AccountID account_id{prefix, value};
    CSV::Table table = make_minimal_table();
    auto md_table = make_md_table(account_id, table);

    auto result = account::statement::md_table_to_account_statement(md_table);

    ASSERT_TRUE(result.has_value())
      << "Failed transformation for prefix: '" << prefix << "'";
    ASSERT_TRUE(result->meta().m_maybe_account_irl_id.has_value())
      << "Missing AccountID for prefix: '" << prefix << "'";
    EXPECT_EQ(result->meta().m_maybe_account_irl_id->m_prefix, prefix)
      << "Prefix mismatch for: '" << prefix << "'";
    EXPECT_EQ(result->meta().m_maybe_account_irl_id->m_value, value)
      << "Value mismatch for prefix: '" << prefix << "'";
  }
}

// ============================================================================
// Return Value Tests
// ============================================================================

TEST_F(MDTableToAccountStatementTestFixture, ReturnsAccountStatementWithEntries) {
  logger::scope_logger log_raii{logger::development_trace,
    "TEST(MDTableToAccountStatement, ReturnsAccountStatementWithEntries)"};

  auto maybe_md_table = make_md_table_from_csv(sz_NORDEA_csv_20251120);
  ASSERT_TRUE(maybe_md_table.has_value());

  auto result = account::statement::md_table_to_account_statement(*maybe_md_table);

  ASSERT_TRUE(result.has_value());

  // Verify we get an AccountStatement type back with accessible entries
  AccountStatement const& statement = *result;
  EXPECT_GT(statement.entries().size(), 0);

  // Verify entries have expected structure
  if (!statement.entries().empty()) {
    auto const& first_entry = statement.entries()[0];
    EXPECT_FALSE(first_entry.transaction_caption.empty());
  }
}

} // namespace tests::md_table_to_account_statement
