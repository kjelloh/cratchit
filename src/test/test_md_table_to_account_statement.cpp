#include "test_atomics.hpp"
#include "test_fixtures.hpp"
#include "logger/log.hpp"
#include "fiscal/amount/AccountStatement.hpp"
#include "domain/csv_to_account_statement.hpp"
#include "csv/csv_to_statement_id_ed.hpp"
#include "csv/parse_csv.hpp"
#include "test/data/account_statements_csv.hpp"
#include <gtest/gtest.h>
#include <optional>
#include <string>

namespace tests {

  namespace statement_id_ed_to_account_statement_step {

    // Test fixture for MDTable -> AccountStatement tests
    class MDTableToAccountStatementTestFixture : public ::testing::Test {
    protected:
      // Helper to create an MDTable from CSV text using the real parsing/detection pipeline
      static std::optional<CSV::MDTable<account::statement::maybe::table::TableMeta>> make_statement_id_ed(
          std::string const& csv_text,
          char delimiter = ';') {
        auto maybe_table = CSV::parse::maybe::csv_to_table(csv_text, delimiter);
        if (!maybe_table) {
          return std::nullopt;
        }
        return account::statement::maybe::to_statement_id_ed_step(*maybe_table);
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

      CSV::Table table = make_minimal_table();
      auto maybe_statement_id_ed = account::statement::maybe::to_statement_id_ed_step(table);

      ASSERT_TRUE(maybe_statement_id_ed);

      auto const& [table_meta,_] = *maybe_statement_id_ed;
      auto provided_account_id = table_meta.account_id;

      // Transform
      auto maybe_statement = account::statement::maybe::statement_id_ed_to_account_statement_step(*maybe_statement_id_ed);

      // Verify AccountID is correctly propagated
      ASSERT_TRUE(maybe_statement.has_value()) << "Expected successful transformation";
      ASSERT_TRUE(maybe_statement->meta().m_maybe_account_irl_id.has_value())
        << "Expected AccountID to be set in meta";
      EXPECT_EQ(maybe_statement->meta().m_maybe_account_irl_id->m_prefix, provided_account_id.m_prefix);
      EXPECT_EQ(maybe_statement->meta().m_maybe_account_irl_id->m_value, provided_account_id.m_value);
      EXPECT_EQ(maybe_statement->meta().m_maybe_account_irl_id->to_string(), provided_account_id.to_string());
    }

    TEST_F(MDTableToAccountStatementTestFixture, SKVAccountIdPropagatedCorrectly) {
      logger::scope_logger log_raii{logger::development_trace,
        "TEST(MDTableToAccountStatement, SKVAccountIdPropagatedCorrectly)"};

      auto maybe_statement_id_ed = make_statement_id_ed(sz_SKV_csv_20251120);

      ASSERT_TRUE(maybe_statement_id_ed);

      auto const& [table_meta,_] = *maybe_statement_id_ed;
      auto provided_account_id = table_meta.account_id;

      // Transform
      auto maybe_statement = account::statement::maybe::statement_id_ed_to_account_statement_step(*maybe_statement_id_ed);

      // Verify AccountID is correctly propagated
      ASSERT_TRUE(maybe_statement.has_value()) << "Expected successful transformation";
      ASSERT_TRUE(maybe_statement->meta().m_maybe_account_irl_id.has_value())
        << "Expected AccountID to be set in meta";
      EXPECT_EQ(maybe_statement->meta().m_maybe_account_irl_id->m_prefix, provided_account_id.m_prefix);
      EXPECT_EQ(maybe_statement->meta().m_maybe_account_irl_id->m_value, provided_account_id.m_value);
      EXPECT_EQ(maybe_statement->meta().m_maybe_account_irl_id->to_string(), provided_account_id.to_string());
    }

    // ============================================================================
    // Integration Tests - Using Real CSV Data Through Full Detection Pipeline
    // ============================================================================

    TEST_F(MDTableToAccountStatementTestFixture, NordeaMDTableToAccountStatement) {
      logger::scope_logger log_raii{logger::development_trace,
        "TEST(MDTableToAccountStatement, NordeaMDTableToAccountStatement)"};

      // Parse NORDEA CSV and detect account
      auto maybe_statement_id_ed = make_statement_id_ed(sz_NORDEA_csv_20251120);

      ASSERT_TRUE(maybe_statement_id_ed.has_value()) << "Expected successful MDTable creation";

      // Verify detected AccountID before transformation
      EXPECT_EQ(maybe_statement_id_ed->meta.account_id.m_prefix, "NORDEA") << "Expected Account ID";

      // Transform MDTable to AccountStatement
      auto result = account::statement::maybe::statement_id_ed_to_account_statement_step(*maybe_statement_id_ed);

      ASSERT_TRUE(result.has_value()) << "Expected successful transformation";

      // Verify AccountID propagated correctly
      ASSERT_TRUE(result->meta().m_maybe_account_irl_id.has_value()) << "Expected account ID";
      EXPECT_EQ(result->meta().m_maybe_account_irl_id->m_prefix, "NORDEA") << "Expected NORDEA prefix";

      // Verify entries were created
      EXPECT_GT(result->entries().size(), 0) << "Expected at least one entry";
    }

    TEST_F(MDTableToAccountStatementTestFixture, SKVOlderMDTableToAccountStatement) {
      logger::scope_logger log_raii{logger::development_trace,
        "TEST(MDTableToAccountStatement, SKVOlderMDTableToAccountStatement)"};

      // Parse SKV CSV (older format) and detect account
      auto maybe_statement_id_ed = make_statement_id_ed(sz_SKV_csv_older);

      ASSERT_TRUE(maybe_statement_id_ed.has_value()) << "Expected successful MDTable creation";

      // Verify detected AccountID
      EXPECT_EQ(maybe_statement_id_ed->meta.account_id.m_prefix, "SKV");

      // Transform MDTable to AccountStatement
      auto result = account::statement::maybe::statement_id_ed_to_account_statement_step(*maybe_statement_id_ed);

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

      auto maybe_statement_id_ed = account::statement::maybe::to_statement_id_ed_step(*maybe_table);
      ASSERT_TRUE(maybe_statement_id_ed.has_value()) << "Expected successful MDTable creation";

      // Verify detected AccountID (should find org number 5567828172)
      EXPECT_EQ(maybe_statement_id_ed->meta.account_id.m_prefix, "SKV");

      // Transform MDTable to AccountStatement
      auto result = account::statement::maybe::statement_id_ed_to_account_statement_step(*maybe_statement_id_ed);

      ASSERT_TRUE(result.has_value()) << "Expected successful transformation";

      // Verify AccountID propagated
      ASSERT_TRUE(result->meta().m_maybe_account_irl_id.has_value());
      EXPECT_EQ(result->meta().m_maybe_account_irl_id->m_prefix, "SKV");
    }

    // ============================================================================
    // Return Value Tests
    // ============================================================================

    TEST_F(MDTableToAccountStatementTestFixture, ReturnsAccountStatementWithEntries) {
      logger::scope_logger log_raii{logger::development_trace,
        "TEST(MDTableToAccountStatement, ReturnsAccountStatementWithEntries)"};

      auto maybe_statement_id_ed = make_statement_id_ed(sz_NORDEA_csv_20251120);
      ASSERT_TRUE(maybe_statement_id_ed.has_value());

      auto result = account::statement::maybe::statement_id_ed_to_account_statement_step(*maybe_statement_id_ed);

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

  } // statement_id_ed_to_account_statement_step
}
