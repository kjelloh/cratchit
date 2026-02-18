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

    // BEGIN TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok sub tests

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok_sub_0) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok_sub_0)"};
      std::string caption = "sz_NORDEA_csv_20251120";
      std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("EWxpected {} -> Table OK",caption);

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);

      using namespace account::statement;
      // Empty: 10 Text: 0 1 2 3 4 5 6 7 8 9
      static const RowMap EXPECTED_ROW_0_MAP{
        .ixs = {
           {FieldType::Empty,{10}}
          ,{FieldType::Text,{0,1,2,3,4,5,6,7,8,9}}
        }
      };
      ASSERT_TRUE(maybe_statement_mapping->m_row_0_map == EXPECTED_ROW_0_MAP) << std::format(
        "Expected row map:'{}' but got:'{}' for {}"
        ,to_string(EXPECTED_ROW_0_MAP)
        ,to_string(maybe_statement_mapping->m_row_0_map)
        ,caption);
      ASSERT_TRUE(maybe_statement_mapping->has_heading) << std::format("Expected heading for {}",caption);
      ASSERT_FALSE(maybe_statement_mapping->m_maybe_in_out_saldos) << std::format("Expected NO saldos for {}",caption);
      ASSERT_TRUE(maybe_statement_mapping->entry_amounts_type == EntryAmountsType::TransThenSaldo) << std::format(
        "Expected TransThenSaldo bit got {} for {}"
        ,static_cast<int>(maybe_statement_mapping->entry_amounts_type)
        ,caption);
      ASSERT_TRUE(maybe_statement_mapping->maybe_common) << std::format("Expected maybe_common for {}",caption);
      {
        // Empty: 2 3 7 10 Date: 0 Amount: 1 8 Text: 4 5 9
        static const RowMap EXPECTED_COMMON_ROW_MAP{
          .ixs = {
             {FieldType::Empty,{2,3,7,10}}
            ,{FieldType::Date,{0}}
            ,{FieldType::Amount,{1,8}}
            ,{FieldType::Text,{4,5,9}}
          }
        };
        ASSERT_TRUE(maybe_statement_mapping->maybe_common == EXPECTED_COMMON_ROW_MAP) << std::format(
          "Expected common row map:'{}' but got:'{}' for {}"
          ,to_string(EXPECTED_COMMON_ROW_MAP)
          ,to_string(*maybe_statement_mapping->maybe_common)
          ,caption);

      }

    }

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok_sub_1) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok_sub_1)"};
      std::string caption = "sz_NORDEA_csv_20251120";
      std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("EWxpected {} -> Table OK",caption);

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);
      CSV::MDTable<account::statement::StatementMapping>  mapped_table{*maybe_statement_mapping,*maybe_table};
      auto maybe_column_mapping = account::statement::maybe::table::generic_like_to_column_mapping(mapped_table);
      ASSERT_TRUE(maybe_column_mapping) << std::format("Expected valid column mapping for {}",caption);
    }
    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok_sub_2) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok__sub_2)"};
      std::string caption = "sz_NORDEA_csv_20251120";
      std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("EWxpected {} -> Table OK",caption);

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);
      CSV::MDTable<account::statement::StatementMapping>  mapped_table{*maybe_statement_mapping,*maybe_table};
      auto maybe_column_mapping = account::statement::maybe::table::generic_like_to_column_mapping(mapped_table);
      ASSERT_TRUE(maybe_column_mapping) << std::format("Expected valid column mapping for {}",caption);
      auto maybe_account_id = account::statement::maybe::table::generic_like_to_account_id(mapped_table);
      ASSERT_TRUE(maybe_account_id) << std::format("Expected valid account ID for {}",caption);

    }
    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok_sub_3) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok_sub_3)"};
      std::string caption = "sz_NORDEA_csv_20251120";
      std::string csv_text = sz_NORDEA_csv_20251120;
      // std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("EWxpected {} -> Table OK",caption);

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);
      ASSERT_TRUE(statement_table_meta.column_mapping.is_valid()) << std::format("Expected Valid Mapping for {}",caption);

    }

    // END TableMetaBasedGeneric_sz_NORDEA_csv_20251120_Ok sub tests

    // BEGIN TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok sub tests

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok_sub_0) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok_sub_0)"};
      std::string caption = "sz_SKV_csv_20251120";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("EWxpected {} -> Table OK",caption);

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);

      using namespace account::statement;
      // Empty: 2 3 OrgNo: 1 Text: 0
      static const RowMap EXPECTED_ROW_0_MAP{
        .ixs = {
           {FieldType::Empty,{2,3}}
          ,{FieldType::OrgNo,{1}}
          ,{FieldType::Text,{0}}
        }
      };
      ASSERT_TRUE(maybe_statement_mapping->m_row_0_map == EXPECTED_ROW_0_MAP) << std::format(
        "Expected row map:'{}' but got:'{}' for {}"
        ,to_string(EXPECTED_ROW_0_MAP)
        ,to_string(maybe_statement_mapping->m_row_0_map)
        ,caption);
      ASSERT_FALSE(maybe_statement_mapping->has_heading) << std::format("Expected NO heading for {}",caption);
      ASSERT_TRUE(maybe_statement_mapping->m_maybe_in_out_saldos) << std::format("Expected saldos for {}",caption);
      {
        int const SALDO_RIX = 2;
        auto saldo_rix = maybe_statement_mapping->m_maybe_in_out_saldos->m_in_saldo.rix();
        ASSERT_TRUE(saldo_rix == SALDO_RIX) 
          << std::format(
                "Expected in saldo rix:{} but got: {} for {}"
                ,SALDO_RIX
                ,saldo_rix
                ,caption);
      }
      {
        CentsAmount CENTS_SALDO{65800};
        auto cents_saldo = maybe_statement_mapping->m_maybe_in_out_saldos->m_in_saldo.ta().cents_amount();
        ASSERT_TRUE(cents_saldo == CENTS_SALDO) 
          << std::format(
                "Expected in saldo:{} but got:{} for {}"
                ,to_string(CENTS_SALDO)
                ,to_string(cents_saldo)
                ,caption);
      }
      {
        CentsAmount CENTS_SALDO{66000};
        auto cents_saldo = maybe_statement_mapping->m_maybe_in_out_saldos->m_out_saldo.ta().cents_amount();
        ASSERT_TRUE(cents_saldo == CENTS_SALDO) 
          << std::format(
                "Expected out saldo:{} but got:{} for {}"
                ,to_string(CENTS_SALDO)
                ,to_string(cents_saldo)
                ,caption);
      }
      {
        int const SALDO_RIX = 7;
        auto saldo_rix = maybe_statement_mapping->m_maybe_in_out_saldos->m_out_saldo.rix();
        ASSERT_TRUE(saldo_rix == SALDO_RIX) 
          << std::format(
                "Expected out saldo rix:{} but got: {} for {}"
                ,SALDO_RIX
                ,saldo_rix
                ,caption);
      }
      ASSERT_TRUE(maybe_statement_mapping->entry_amounts_type == EntryAmountsType::TransThenSaldo) << std::format(
        "Expected TransThenSaldo bit got {} for {}"
        ,static_cast<int>(maybe_statement_mapping->entry_amounts_type)
        ,caption);
      ASSERT_TRUE(maybe_statement_mapping->maybe_common) << std::format("Expected maybe_common for {}",caption);
      {
        // Date: 0 Amount: 2 3 Text: 1
        static const RowMap EXPECTED_COMMON_ROW_MAP{
          .ixs = {
            {FieldType::Date,{0}}
            ,{FieldType::Amount,{2,3}}
            ,{FieldType::Text,{1}}
          }
        };
        ASSERT_TRUE(maybe_statement_mapping->maybe_common == EXPECTED_COMMON_ROW_MAP) << std::format(
          "Expected common row map:'{}' but got:'{}' for {}"
          ,to_string(EXPECTED_COMMON_ROW_MAP)
          ,to_string(*maybe_statement_mapping->maybe_common)
          ,caption);

      }

    }

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok_sub_1) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok_sub_1)"};
      std::string caption = "sz_SKV_csv_20251120";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("EWxpected {} -> Table OK",caption);

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);
      CSV::MDTable<account::statement::StatementMapping>  mapped_table{*maybe_statement_mapping,*maybe_table};
      auto maybe_column_mapping = account::statement::maybe::table::generic_like_to_column_mapping(mapped_table);
      ASSERT_TRUE(maybe_column_mapping) << std::format("Expected valid column mapping for {}",caption);

    }
    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok_sub_2) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok_sub_2)"};
      std::string caption = "sz_SKV_csv_20251120";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("EWxpected {} -> Table OK",caption);

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);
      CSV::MDTable<account::statement::StatementMapping>  mapped_table{*maybe_statement_mapping,*maybe_table};
      auto maybe_column_mapping = account::statement::maybe::table::generic_like_to_column_mapping(mapped_table);
      ASSERT_TRUE(maybe_column_mapping) << std::format("Expected valid column mapping for {}",caption);
      auto maybe_account_id = account::statement::maybe::table::generic_like_to_account_id(mapped_table);
      ASSERT_TRUE(maybe_account_id) << std::format("Expected valid account ID for {}",caption);

    }
    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok_sub_3) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok_sub_3)"};
      std::string caption = "sz_SKV_csv_20251120";
      // std::string csv_text = sz_NORDEA_csv_20251120;
      std::string csv_text = sz_SKV_csv_20251120;
      // std::string csv_text = sz_SKV_csv_older;
      // std::string csv_text = sz_NORDEA_0_1;
      // std::string csv_text = sz_SKV_0_0;
      // std::string csv_text = sz_SKV_0_0_BOM_ed;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << std::format("EWxpected {} -> Table OK",caption);

      auto statement_table_meta = account::statement::maybe::table::generic_like_to_statement_table_meta(*maybe_table);
      ASSERT_TRUE(statement_table_meta.column_mapping.is_valid()) << std::format("Expected Valid Mapping for {}",caption);
    }


    // END TableMetaBasedGeneric_sz_SKV_csv_20251120_Ok sub tests

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

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);

    }

    // BEGIN TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_x

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_0) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_0)"};
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

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);

      using namespace account::statement;
      // Empty: 0 4 Amount: 2 3 Text: 1
      static const RowMap EXPECTED_ROW_0_MAP{
        .ixs = {
           {FieldType::Empty,{0,4}}
          ,{FieldType::Amount,{2,3}}
          ,{FieldType::Text,{1}}
        }
      };
      ASSERT_TRUE(maybe_statement_mapping->m_row_0_map == EXPECTED_ROW_0_MAP) << std::format(
        "Expected row map:'{}' but got:'{}' for {}"
        ,to_string(EXPECTED_ROW_0_MAP)
        ,to_string(maybe_statement_mapping->m_row_0_map)
        ,caption);
      ASSERT_FALSE(maybe_statement_mapping->has_heading) << std::format("Expected NO heading for {}",caption);
      ASSERT_TRUE(maybe_statement_mapping->m_maybe_in_out_saldos) << std::format("Expected saldos for {}",caption);
      {
        int const SALDO_RIX = 0;
        auto saldo_rix = maybe_statement_mapping->m_maybe_in_out_saldos->m_in_saldo.rix();
        ASSERT_TRUE(saldo_rix == SALDO_RIX) 
          << std::format(
                "Expected in saldo rix:{} but got: {} for {}"
                ,SALDO_RIX
                ,saldo_rix
                ,caption);
      }
      {
        CentsAmount CENTS_SALDO{65600};
        auto cents_saldo = maybe_statement_mapping->m_maybe_in_out_saldos->m_in_saldo.ta().cents_amount();
        ASSERT_TRUE(cents_saldo == CENTS_SALDO) 
          << std::format(
                "Expected in saldo:{} but got:{} for {}"
                ,to_string(CENTS_SALDO)
                ,to_string(cents_saldo)
                ,caption);
      }
      {
        CentsAmount CENTS_SALDO{65800};
        auto cents_saldo = maybe_statement_mapping->m_maybe_in_out_saldos->m_out_saldo.ta().cents_amount();
        ASSERT_TRUE(cents_saldo == CENTS_SALDO) 
          << std::format(
                "Expected out saldo:{} but got:{} for {}"
                ,to_string(CENTS_SALDO)
                ,to_string(cents_saldo)
                ,caption);
      }
      {
        int const SALDO_RIX = 5;
        auto saldo_rix = maybe_statement_mapping->m_maybe_in_out_saldos->m_out_saldo.rix();
        ASSERT_TRUE(saldo_rix == SALDO_RIX) 
          << std::format(
                "Expected out saldo rix:{} but got: {} for {}"
                ,SALDO_RIX
                ,saldo_rix
                ,caption);
      }
      ASSERT_TRUE(maybe_statement_mapping->entry_amounts_type == EntryAmountsType::TransOnly) << std::format(
        "Expected TransThenSaldo bit got {} for {}"
        ,static_cast<int>(maybe_statement_mapping->entry_amounts_type)
        ,caption);
      ASSERT_TRUE(maybe_statement_mapping->maybe_common) << std::format("Expected maybe_common for {}",caption);
      {
        // Empty: 3 4 Date: 0 Amount: 2 Text: 1
        static const RowMap EXPECTED_COMMON_ROW_MAP{
          .ixs = {
             {FieldType::Empty,{3,4}}
            ,{FieldType::Date,{0}}
            ,{FieldType::Amount,{2}}
            ,{FieldType::Text,{1}}
          }
        };
        ASSERT_TRUE(maybe_statement_mapping->maybe_common == EXPECTED_COMMON_ROW_MAP) << std::format(
          "Expected common row map:'{}' but got:'{}' for {}"
          ,to_string(EXPECTED_COMMON_ROW_MAP)
          ,to_string(*maybe_statement_mapping->maybe_common)
          ,caption);

      }


    } // TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_0

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_1) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_1)"};
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

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);
      CSV::MDTable<account::statement::StatementMapping>  mapped_table{*maybe_statement_mapping,*maybe_table};
      auto maybe_column_mapping = account::statement::maybe::table::generic_like_to_column_mapping(mapped_table);
      ASSERT_TRUE(maybe_column_mapping) << std::format("Expected valid column mapping for {}",caption);

    } // TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_1
    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_2) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_2)"};
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

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);
      CSV::MDTable<account::statement::StatementMapping>  mapped_table{*maybe_statement_mapping,*maybe_table};
      auto maybe_column_mapping = account::statement::maybe::table::generic_like_to_column_mapping(mapped_table);
      ASSERT_TRUE(maybe_column_mapping) << std::format("Expected valid column mapping for {}",caption);
      auto maybe_account_id = account::statement::maybe::table::generic_like_to_account_id(mapped_table);
      ASSERT_TRUE(maybe_account_id) << std::format("Expected valid account ID for {}",caption);

    } // TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_2
    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_3) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_3)"};
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

    } // TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_3

    // END TableMetaBasedGeneric_sz_SKV_csv_older_Ok_sub_x

    // BEGIN TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_x

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_0) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_0)"};
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


      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);

      using namespace account::statement;
      // Empty: 10 Text: 0 1 2 3 4 5 6 7 8 9
      static const RowMap EXPECTED_ROW_0_MAP{
        .ixs = {
           {FieldType::Empty,{10}}
          ,{FieldType::Text,{0,1,2,3,4,5,6,7,8,9}}
        }
      };
      ASSERT_TRUE(maybe_statement_mapping->m_row_0_map == EXPECTED_ROW_0_MAP) << std::format(
        "Expected row map:'{}' but got:'{}' for {}"
        ,to_string(EXPECTED_ROW_0_MAP)
        ,to_string(maybe_statement_mapping->m_row_0_map)
        ,caption);
      ASSERT_TRUE(maybe_statement_mapping->has_heading) << std::format("Expected heading for {}",caption);
      ASSERT_FALSE(maybe_statement_mapping->m_maybe_in_out_saldos) << std::format("Expected NO saldos for {}",caption);
      ASSERT_TRUE(maybe_statement_mapping->entry_amounts_type == EntryAmountsType::TransThenSaldo) << std::format(
        "Expected TransThenSaldo bit got {} for {}"
        ,static_cast<int>(maybe_statement_mapping->entry_amounts_type)
        ,caption);
      ASSERT_TRUE(maybe_statement_mapping->maybe_common) << std::format("Expected maybe_common for {}",caption);
      {
        // Empty: 2 3 6 7 10 Date: 0 Amount: 1 8 Text: 4 5 9
        static const RowMap EXPECTED_COMMON_ROW_MAP{
          .ixs = {
             {FieldType::Empty,{2,3,6,7,10}}
            ,{FieldType::Date,{0}}
            ,{FieldType::Amount,{1,8}}
            ,{FieldType::Text,{4,5,9}}
          }
        };
        ASSERT_TRUE(maybe_statement_mapping->maybe_common == EXPECTED_COMMON_ROW_MAP) << std::format(
          "Expected common row map:'{}' but got:'{}' for {}"
          ,to_string(EXPECTED_COMMON_ROW_MAP)
          ,to_string(*maybe_statement_mapping->maybe_common)
          ,caption);

      }

    } // TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_0

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_1) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_1)"};
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

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);
      CSV::MDTable<account::statement::StatementMapping>  mapped_table{*maybe_statement_mapping,*maybe_table};
      auto maybe_column_mapping = account::statement::maybe::table::generic_like_to_column_mapping(mapped_table);
      ASSERT_TRUE(maybe_column_mapping) << std::format("Expected valid column mapping for {}",caption);

    } // TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_1
    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_2) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_2)"};
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

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);
      CSV::MDTable<account::statement::StatementMapping>  mapped_table{*maybe_statement_mapping,*maybe_table};
      auto maybe_column_mapping = account::statement::maybe::table::generic_like_to_column_mapping(mapped_table);
      ASSERT_TRUE(maybe_column_mapping) << std::format("Expected valid column mapping for {}",caption);
      auto maybe_account_id = account::statement::maybe::table::generic_like_to_account_id(mapped_table);
      ASSERT_TRUE(maybe_account_id) << std::format("Expected valid account ID for {}",caption);

    } // TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_2
    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_3) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_3)"};
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

    } // TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_3

    // END TableMetaBasedGeneric_sz_NORDEA_0_1_Ok_sub_x

    // BEGIN TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_x

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_0) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_0)"};
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

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);

      using namespace account::statement;
      // Empty: 2 3 OrgNo: 1 Text: 0
      static const RowMap EXPECTED_ROW_0_MAP{
        .ixs = {
           {FieldType::Empty,{2,3}}
          ,{FieldType::OrgNo,{1}}
          ,{FieldType::Text,{0}}
        }
      };
      ASSERT_TRUE(maybe_statement_mapping->m_row_0_map == EXPECTED_ROW_0_MAP) << std::format(
        "Expected row map:'{}' but got:'{}' for {}"
        ,to_string(EXPECTED_ROW_0_MAP)
        ,to_string(maybe_statement_mapping->m_row_0_map)
        ,caption);
      ASSERT_FALSE(maybe_statement_mapping->has_heading) << std::format("Expected NO heading for {}",caption);
      ASSERT_TRUE(maybe_statement_mapping->m_maybe_in_out_saldos) << std::format("Expected saldos for {}",caption);
      {
        int const SALDO_RIX = 2;
        auto saldo_rix = maybe_statement_mapping->m_maybe_in_out_saldos->m_in_saldo.rix();
        ASSERT_TRUE(saldo_rix == SALDO_RIX) 
          << std::format(
                "Expected in saldo rix:{} but got: {} for {}"
                ,SALDO_RIX
                ,saldo_rix
                ,caption);
      }
      {
        CentsAmount CENTS_SALDO{66300};
        auto cents_saldo = maybe_statement_mapping->m_maybe_in_out_saldos->m_in_saldo.ta().cents_amount();
        ASSERT_TRUE(cents_saldo == CENTS_SALDO) 
          << std::format(
                "Expected in saldo:{} but got:{} for {}"
                ,to_string(CENTS_SALDO)
                ,to_string(cents_saldo)
                ,caption);
      }
      {
        CentsAmount CENTS_SALDO{66300};
        auto cents_saldo = maybe_statement_mapping->m_maybe_in_out_saldos->m_out_saldo.ta().cents_amount();
        ASSERT_TRUE(cents_saldo == CENTS_SALDO) 
          << std::format(
                "Expected out saldo:{} but got:{} for {}"
                ,to_string(CENTS_SALDO)
                ,to_string(cents_saldo)
                ,caption);
      }
      {
        int const SALDO_RIX = 3;
        auto saldo_rix = maybe_statement_mapping->m_maybe_in_out_saldos->m_out_saldo.rix();
        ASSERT_TRUE(saldo_rix == SALDO_RIX) 
          << std::format(
                "Expected out saldo rix:{} but got: {} for {}"
                ,SALDO_RIX
                ,saldo_rix
                ,caption);
      }
      ASSERT_TRUE(maybe_statement_mapping->entry_amounts_type == EntryAmountsType::Undefined) << std::format(
        "Expected TransThenSaldo bit got {} for {}"
        ,static_cast<int>(maybe_statement_mapping->entry_amounts_type)
        ,caption);
      ASSERT_FALSE(maybe_statement_mapping->maybe_common) << std::format("Expected NO maybe_common for {}",caption);

    } // TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_0

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_1) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_1)"};
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

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);
      CSV::MDTable<account::statement::StatementMapping>  mapped_table{*maybe_statement_mapping,*maybe_table};
      auto maybe_column_mapping = account::statement::maybe::table::generic_like_to_column_mapping(mapped_table);
      ASSERT_FALSE(maybe_column_mapping) << std::format("Expected NO column mapping for {}",caption);

    } // TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_1

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_2) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_2)"};
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

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);
      CSV::MDTable<account::statement::StatementMapping>  mapped_table{*maybe_statement_mapping,*maybe_table};
      auto maybe_column_mapping = account::statement::maybe::table::generic_like_to_column_mapping(mapped_table);
      ASSERT_FALSE(maybe_column_mapping) << std::format("Expected NO column mapping for {}",caption);
      auto maybe_account_id = account::statement::maybe::table::generic_like_to_account_id(mapped_table);
      ASSERT_TRUE(maybe_account_id) << std::format("Expected valid account ID for {}",caption);

    } // TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_2

    TEST_F(AccountStatementTableTestsFixture,TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_3) {
      logger::scope_logger log_raii{logger::development_trace, "TEST_F(AccountStatementTableTestsFixture, TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_3)"};
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
      ASSERT_FALSE(statement_table_meta.column_mapping.is_valid()) << std::format("Expected NO Mapping for {}",caption);

    } // TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_3

    // END TableMetaBasedGeneric_sz_SKV_0_0_Ok_sub_x

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

      auto maybe_statement_mapping = account::statement::maybe::table::generic_like_to_statement_mapping(*maybe_table);
      ASSERT_TRUE(maybe_statement_mapping) << std::format("Expected valid statement mapping for {}",caption);

    } // TableMetaBasedGeneric_sz_SKV_0_0_BOM_ed_Ok

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
