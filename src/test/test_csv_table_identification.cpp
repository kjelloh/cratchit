#include "test_atomics.hpp"
#include "test_fixtures.hpp"
#include "test/data/account_statements_csv.hpp"
#include "logger/log.hpp"
#include "csv/import_pipeline.hpp"
#include "domain/csv_to_account_statement.hpp"
#include <gtest/gtest.h>

namespace tests::csv_table_identification {

  namespace account_statement_table_suite {

    TEST(AccountStatementDetectionTests, GeneratedSingleRowMappingTest) {
      EXPECT_TRUE(false) << std::format("TODO: Implement GeneratedSingleRowMappingTest");   
    }

    TEST(AccountStatementDetectionTests, GeneratedRowsMappingTest) {
      EXPECT_TRUE(false) << std::format("TODO: Implement GeneratedRowsMappingTest");   
    }

    TEST(AccountStatementDetectionTests, DetectColumnsFromHeader) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementDetectionTests, DetectColumnsFromHeader)"};

      // Create a simple CSV table with headers
      CSV::Table table;
      table.heading = Key::Path{std::vector<std::string>{"Date", "Amount", "Description"}};
      table.rows.push_back(table.heading);
      table.rows.push_back(Key::Path{std::vector<std::string>{"2025-01-01", "100.50", "Test Transaction"}});

      // Detect columns
      auto mapping = account::statement::maybe::table::detect_columns_from_header(table.heading);

      EXPECT_TRUE(mapping.is_valid()) << "Expected valid column mapping";
      EXPECT_EQ(mapping.date_column, 0);
      EXPECT_EQ(mapping.transaction_amount_column, 1);
      EXPECT_EQ(mapping.description_column, 2);
    }

    TEST(AccountStatementDetectionTests, DetectColumnsFromNordeaHeader) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementDetectionTests, DetectColumnsFromNordeaHeader)"};

      std::string csv_text = sz_NORDEA_csv_20251120;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << "Failed to parse NORDEA CSV";

      auto maybe_md_table = account::statement::maybe::to_account_id_ed_step(*maybe_table);
      ASSERT_TRUE(maybe_md_table.has_value()) << "Failed to extract AccountID from NORDEA CSV";

      auto mapping = account::statement::maybe::table::detect_columns_from_header(maybe_md_table->defacto.heading);

      EXPECT_TRUE(mapping.is_valid()) << "Expected valid column mapping from data analysis";
    }

    TEST(AccountStatementDetectionTests, DetectColumnsFromData) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementDetectionTests, DetectColumnsFromData)"};

      // Create a CSV table without meaningful headers
      CSV::Table table;
      table.heading = Key::Path{std::vector<std::string>{"Col0", "Col1", "Col2"}};
      table.rows.push_back(Key::Path{std::vector<std::string>{"2025-01-01", "Payment received", "100.50"}});
      table.rows.push_back(Key::Path{std::vector<std::string>{"2025-01-02", "Withdrawal", "-50.00"}});
      table.rows.push_back(Key::Path{std::vector<std::string>{"2025-01-03", "Transfer", "200.00"}});

      // Detect columns from data patterns
      auto mapping = account::statement::maybe::table::detect_columns_from_data(table.rows);

      EXPECT_TRUE(mapping.is_valid()) << "Expected valid column mapping from data analysis";
      EXPECT_EQ(mapping.date_column, 0) << "Expected date in column 0";
      EXPECT_EQ(mapping.transaction_amount_column, 2) << "Expected transaction amount in column 2";
      EXPECT_EQ(mapping.description_column, 1) << "Expected description in column 1";
    }

    TEST(AccountStatementDetectionTests, DetectColumnsFromSKVNewData) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementDetectionTests, DetectColumnsFromSKVNewData)"};

      std::string csv_text = sz_SKV_csv_20251120;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << "Failed to parse SKV CSV";

      auto maybe_md_table = account::statement::maybe::to_account_id_ed_step(*maybe_table);
      ASSERT_TRUE(maybe_md_table.has_value()) << "Failed to extract AccountID from SKV CSV";

      auto mapping = account::statement::maybe::table::detect_columns_from_data(maybe_md_table->defacto.rows);

      EXPECT_TRUE(mapping.is_valid()) << "Expected valid column mapping from data analysis";
      EXPECT_EQ(mapping.date_column, 0) << "Expected date in column 0";
      EXPECT_EQ(mapping.transaction_amount_column, 2) << "Expected transaction amount in column 2";
      EXPECT_EQ(mapping.description_column, 1) << "Expected description in column 1";

    }

    TEST(AccountStatementTests, InvalidDateHandledGracefully) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, InvalidDateHandledGracefully)"};

      // Create a CSV table with invalid date
      CSV::Table table;
      table.heading = Key::Path{std::vector<std::string>{"Date", "Amount", "Description"}};
      table.rows.push_back(table.heading);
      table.rows.push_back(Key::Path{std::vector<std::string>{"not-a-date", "100.50", "Test"}});

      auto mapping = account::statement::maybe::table::detect_columns_from_header(table.heading);
      auto maybe_entry = account::statement::maybe::table::extract_entry_from_row(table.rows[1], mapping);

      EXPECT_FALSE(maybe_entry.has_value()) << "Expected nullopt for invalid date";
    }

    TEST(AccountStatementTests, InvalidAmountHandledGracefully) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, InvalidAmountHandledGracefully)"};

      // Create a CSV table with invalid amount
      CSV::Table table;
      table.heading = Key::Path{std::vector<std::string>{"Date", "Amount", "Description"}};
      table.rows.push_back(table.heading);
      table.rows.push_back(Key::Path{std::vector<std::string>{"2025-01-01", "not-an-amount", "Test"}});

      auto mapping = account::statement::maybe::table::detect_columns_from_header(table.heading);
      auto maybe_entry = account::statement::maybe::table::extract_entry_from_row(table.rows[1], mapping);

      EXPECT_FALSE(maybe_entry.has_value()) << "Expected nullopt for invalid amount";
    }

    TEST(AccountStatementTests, EmptyDescriptionHandledGracefully) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, EmptyDescriptionHandledGracefully)"};

      // Create a CSV table with empty description
      CSV::Table table;
      table.heading = Key::Path{std::vector<std::string>{"Date", "Amount", "Description"}};
      table.rows.push_back(table.heading);
      table.rows.push_back(Key::Path{std::vector<std::string>{"2025-01-01", "100.50", ""}});

      auto mapping = account::statement::maybe::table::detect_columns_from_header(table.heading);
      auto maybe_entry = account::statement::maybe::table::extract_entry_from_row(table.rows[1], mapping);

      EXPECT_FALSE(maybe_entry.has_value()) << "Expected nullopt for empty description";
    }

    TEST(AccountStatementTests, DetectBothAmountColumns) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTests, DetectBothAmountColumns)"};

      // Parse SKV CSV to get a table, then verify column detection
      std::string csv_text = sz_SKV_csv_20251120;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);

      ASSERT_TRUE(maybe_table.has_value()) << "Expected successful CSV parse";

      // Detect columns from data
      auto mapping = account::statement::maybe::table::detect_columns_from_data(maybe_table->rows);

      EXPECT_TRUE(mapping.is_valid()) << "Expected valid column mapping";

      // SKV format: Col0=Date, Col1=Description, Col2=Transaction, Col3=Saldo
      EXPECT_EQ(mapping.date_column, 0) << "Expected date in column 0";
      EXPECT_EQ(mapping.description_column, 1) << "Expected description in column 1";
      EXPECT_EQ(mapping.transaction_amount_column, 2) << "Expected transaction amount in column 2";
      EXPECT_EQ(mapping.saldo_amount_column, 3) << "Expected saldo amount in column 3";

      logger::development_trace("Column mapping: date={}, desc={}, trans_amt={}, saldo={}",
        mapping.date_column, mapping.description_column,
        mapping.transaction_amount_column, mapping.saldo_amount_column);
    }

    
  } // account_statement_table_suite
} // csv_table_identification
