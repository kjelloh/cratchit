#include "test_atomics.hpp"
#include "test_fixtures.hpp"
#include "test/data/account_statements_csv.hpp"
#include "logger/log.hpp"
#include "csv/import_pipeline.hpp"
#include "domain/csv_to_account_statement.hpp"
#include <gtest/gtest.h>

namespace tests::csv_table_identification {

  namespace account_statement_table_suite {

    class AccountStatementTableTestsFixture : public ::testing::Test {
    protected:
      // Helper to create a minimal CSV::Table with valid structure
      static CSV::Table to_minimal_table() {
        CSV::Table table;
        table.heading = Key::Path(std::vector<std::string>{"Datum", "Belopp", "Namn"});
        table.rows = {
          Key::Path(std::vector<std::string>{"Datum", "Belopp", "Namn"}),  // Header row (duplicated as per CSV::Table convention)
          Key::Path(std::vector<std::string>{"2025-01-15", "100,50", "Test Transaction"})
        };
        return table;
      }
    }; // AccountStatementTableTestsFixture

    TEST_F(AccountStatementTableTestsFixture,MappingBasedMinimalParseOK) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, MappingBasedMinimalParseOK)"};
      auto table = to_minimal_table();
      auto mapping = account::statement::maybe::table::to_column_mapping(table);

      ASSERT_TRUE(mapping.is_valid()) << "Expected Valid Mapping";
    }

    TEST_F(AccountStatementTableTestsFixture,MappingBasedNordeaLikeOk) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, MappingBasedNordeaLikeOk)"};
      std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_older;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << "Expetced sz_NORDEA_csv_20251120 -> Table OK";

      auto mapping = account::statement::maybe::table::nordea_like_to_column_mapping(*maybe_table);

      ASSERT_TRUE(mapping.is_valid()) << "Expected Valid Mapping";
    }

    TEST_F(AccountStatementTableTestsFixture,MappingBasedSKVLikeOlderOk) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, MappingBasedSKVLikeOlderOk)"};
      // std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      std::string csv_text = sz_SKV_csv_older;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << "EWxpected sz_SKV_csv_older -> Table OK";

      auto mapping = account::statement::maybe::table::to_column_mapping(*maybe_table);

      ASSERT_TRUE(mapping.is_valid()) << "Expected Valid Mapping";

    }

    TEST_F(AccountStatementTableTestsFixture,MappingBasedSKVLike20251120OOk) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, MappingBasedSKVLike20251120OOk)"};
      // std::string csv_text = sz_NORDEA_csv_20251120;
      std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_older;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << "Expected sz_SKV_csv_20251120 -> Table OK ";

      auto mapping = account::statement::maybe::table::to_column_mapping(*maybe_table);

      ASSERT_TRUE(mapping.is_valid()) << "Expected Valid Mapping";

    }

    // ***********************
    // Test of Generic mapping
    // ***********************

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok)"};
      std::string caption = "sz_SKV_csv_20251120";
      std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string = sz_SKV_csv_20251120_BOM_ed;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("EWxpected {} -> Table OK",caption);

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);

      ASSERT_TRUE(statement_table_meta.column_mapping.is_valid()) << std::format("Expected Valid Mapping for {}",caption);

    }

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok)"};
      std::string caption = "sz_SKV_csv_20251120";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      std::string csv_text = sz_SKV_csv_20251120;
      // std::string = sz_SKV_csv_20251120_BOM_ed;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("EWxpected {} -> Table OK",caption);

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);

      ASSERT_TRUE(statement_table_meta.column_mapping.is_valid()) << std::format("Expected Valid Mapping for {}",caption);

    }

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_csv_20251120_BOM_ed_Ok) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_csv_20251120_BOM_ed_Ok)"};
      std::string caption = "sz_SKV_csv_20251120_BOM_ed";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      std::string csv_text = sz_SKV_csv_20251120_BOM_ed;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("EWxpected {} -> Table OK",caption);

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);

      ASSERT_TRUE(statement_table_meta.column_mapping.is_valid()) << std::format("Expected Valid Mapping for {}",caption);

    }

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_csv_older_Ok) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_csv_older_Ok)"};
      std::string caption = "sz_SKV_csv_older";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120_BOM_ed;
      std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("EWxpected {} -> Table OK",caption);

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);

      ASSERT_TRUE(statement_table_meta.column_mapping.is_valid()) << std::format("Expected Valid Mapping for {}",caption);

    }

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_NORDEA_0_1_Ok) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_NORDEA_0_1_Ok)"};
      std::string caption = "sz_NORDEA_0_1";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120_BOM_ed;
      // std::string csv_text = sz_SKV_csv_older;
      std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("EWxpected {} -> Table OK",caption);

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);

      ASSERT_TRUE(statement_table_meta.column_mapping.is_valid()) << std::format("Expected Valid Mapping for {}",caption);

    }

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_0_0_Ok) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_0_0_Ok)"};
      std::string caption = "sz_SKV_0_0";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120_BOM_ed;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("EWxpected {} -> Table OK",caption);

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);

      ASSERT_TRUE(statement_table_meta.column_mapping.is_valid()) << std::format("Expected Valid Mapping for {}",caption);

    }

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_0_0_BOM_ed_Ok) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_0_0_BOM_ed_Ok)"};
      std::string caption = "sz_SKV_0_0_BOM_ed";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120_BOM_ed;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("EWxpected {} -> Table OK",caption);

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);

      ASSERT_TRUE(statement_table_meta.column_mapping.is_valid()) << std::format("Expected Valid Mapping for {}",caption);

    }

    TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectColumnsFromHeader) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectColumnsFromHeader)"};

      // Create a simple CSV table with headers
      CSV::Table table;
      table.heading = Key::Path{std::vector<std::string>{"Date", "Amount", "Description"}};
      table.rows.push_back(table.heading);
      table.rows.push_back(Key::Path{std::vector<std::string>{"2025-01-01", "100.50", "Test Transaction"}});

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(table);
      auto mapping = statement_table_meta.column_mapping;

      EXPECT_TRUE(mapping.is_valid()) << "Expected valid column mapping";
      EXPECT_EQ(mapping.date_column, 0);
      EXPECT_EQ(mapping.transaction_amount_column, 1);
      EXPECT_EQ(mapping.description_column, 2);
    }

    TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectColumnsFromData) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectColumnsFromData)"};

      // Create a CSV table without meaningful headers
      CSV::Table table;
      table.heading = Key::Path{std::vector<std::string>{"Col0", "Col1", "Col2"}};
      table.rows.push_back(Key::Path{std::vector<std::string>{"2025-01-01", "Payment received", "100.50"}});
      table.rows.push_back(Key::Path{std::vector<std::string>{"2025-01-02", "Withdrawal", "-50.00"}});
      table.rows.push_back(Key::Path{std::vector<std::string>{"2025-01-03", "Transfer", "200.00"}});

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(table);
      auto mapping = statement_table_meta.column_mapping;

      EXPECT_TRUE(mapping.is_valid()) << "Expected valid column mapping from data analysis";
      EXPECT_EQ(mapping.date_column, 0) << "Expected date in column 0";
      EXPECT_EQ(mapping.transaction_amount_column, 2) << "Expected transaction amount in column 2";
      EXPECT_EQ(mapping.description_column, 1) << "Expected description in column 1";
    }

    TEST(AccountStatementTableTests, TableMetaBasedGeneric_InvalidDateHandledGracefully) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTableTests, TableMetaBasedGeneric_InvalidDateHandledGracefully)"};

      // Create a CSV table with invalid date
      CSV::Table table;
      table.heading = Key::Path{std::vector<std::string>{"Date", "Amount", "Description"}};
      table.rows.push_back(table.heading);
      table.rows.push_back(Key::Path{std::vector<std::string>{"not-a-date", "100.50", "Test"}});

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(table);
      auto mapping = statement_table_meta.column_mapping;
      auto maybe_entry = account::statement::maybe::table::extract_entry_from_row(table.rows[1], mapping);

      EXPECT_FALSE(maybe_entry.has_value()) << "Expected nullopt for invalid date";
    }

    TEST(AccountStatementTableTests, TableMetaBasedGeneric_InvalidAmountHandledGracefully) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTableTests, TableMetaBasedGeneric_InvalidAmountHandledGracefully)"};

      // Create a CSV table with invalid amount
      CSV::Table table;
      table.heading = Key::Path{std::vector<std::string>{"Date", "Amount", "Description"}};
      table.rows.push_back(table.heading);
      table.rows.push_back(Key::Path{std::vector<std::string>{"2025-01-01", "not-an-amount", "Test"}});

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(table);
      auto mapping = statement_table_meta.column_mapping;
      auto maybe_entry = account::statement::maybe::table::extract_entry_from_row(table.rows[1], mapping);

      EXPECT_FALSE(maybe_entry.has_value()) << "Expected nullopt for invalid amount";
    }

    TEST(AccountStatementTableTests, TableMetaBasedGeneric_EmptyDescriptionHandledGracefully) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTableTests, TableMetaBasedGeneric_EmptyDescriptionHandledGracefully)"};

      // Create a CSV table with empty description
      CSV::Table table;
      table.heading = Key::Path{std::vector<std::string>{"Date", "Amount", "Description"}};
      table.rows.push_back(table.heading);
      table.rows.push_back(Key::Path{std::vector<std::string>{"2025-01-01", "100.50", ""}});

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(table);
      auto mapping = statement_table_meta.column_mapping;
      auto maybe_entry = account::statement::maybe::table::extract_entry_from_row(table.rows[1], mapping);

      EXPECT_FALSE(maybe_entry.has_value()) << "Expected nullopt for empty description";
    }

    TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectColumnsFromNordeaHeader) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectColumnsFromNordeaHeader)"};

      std::string csv_text = sz_NORDEA_csv_20251120;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << "Failed to parse NORDEA CSV";

      auto maybe_md_table = account::statement::maybe::to_statement_id_ed_step(*maybe_table);
      ASSERT_TRUE(maybe_md_table.has_value()) << "Failed to extract AccountID from NORDEA CSV";

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);
      auto mapping = statement_table_meta.column_mapping;

      EXPECT_TRUE(mapping.is_valid()) << "Expected valid column mapping from data analysis";
    }

    TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectColumnsFromSKVNewData) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectColumnsFromSKVNewData)"};

      std::string csv_text = sz_SKV_csv_20251120;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << "Failed to parse SKV CSV";

      auto maybe_md_table = account::statement::maybe::to_statement_id_ed_step(*maybe_table);
      ASSERT_TRUE(maybe_md_table.has_value()) << "Failed to extract AccountID from SKV CSV";


      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);
      auto mapping = statement_table_meta.column_mapping;

      EXPECT_TRUE(mapping.is_valid()) << "Expected valid column mapping from data analysis";
      EXPECT_EQ(mapping.date_column, 0) << "Expected date in column 0";
      EXPECT_EQ(mapping.transaction_amount_column, 2) << "Expected transaction amount in column 2";
      EXPECT_EQ(mapping.description_column, 1) << "Expected description in column 1";

    }

    TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectBothSKVAmountColumnsOld) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectBothSKVAmountColumnsOld)"};

      // Parse SKV CSV to get a table, then verify column detection
      std::string csv_text = sz_SKV_csv_older;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);

      ASSERT_TRUE(maybe_table.has_value()) << "Expected successful CSV parse";

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);
      auto mapping = statement_table_meta.column_mapping;

      EXPECT_TRUE(mapping.is_valid()) << "Expected valid column mapping";

      EXPECT_EQ(mapping.date_column, 0) << "Expected date in column 0";
      EXPECT_EQ(mapping.description_column, 1) << "Expected description in column 1";
      EXPECT_EQ(mapping.transaction_amount_column, 2) << "Expected transaction amount in column 2";

      logger::development_trace("Column mapping: date={}, desc={}, trans_amt={}",
         mapping.date_column
        ,mapping.description_column
        ,mapping.transaction_amount_column);
    }

    TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectBothSKVAmountColumns251120) {
      logger::scope_logger log_raii{logger::development_trace, "TEST(AccountStatementTableTests, TableMetaBasedGeneric_DetectBothSKVAmountColumns251120)"};

      // Parse SKV CSV to get a table, then verify column detection
      std::string csv_text = sz_SKV_csv_20251120;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);

      ASSERT_TRUE(maybe_table.has_value()) << "Expected successful CSV parse";

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);
      auto mapping = statement_table_meta.column_mapping;

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
