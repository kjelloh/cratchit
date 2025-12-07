#include "logger/log.hpp"
#include "csv/parse_csv.hpp"
#include "csv/csv_to_account_id.hpp"
#include "test/data/account_statements_csv.hpp"
#include <gtest/gtest.h>

namespace tests::generic_statement_csv {

  namespace happy_path_suite  {

    TEST(GenericStatementCSVTests,NORDEAStatementOk) {

      logger::scope_logger log_riia(
         logger::development_trace
        ,"TEST(GenericStatementCSVTests,NORDEAStatementOk)"
        ,logger::LogToConsole::ON);
      
      std::string csv_text = sz_NORDEA_csv_20251120;
      auto maybe_table = CSV::parse::maybe::csv_text_to_table_step(csv_text);
      ASSERT_TRUE(maybe_table.has_value()) << "Expected successful parse of Nordea CSV";

      auto maybe_entries_mapped = account::statement::maybe::to_entries_mapped(*maybe_table);
      ASSERT_TRUE(maybe_entries_mapped.has_value()) << "Expected succesfull recognition of NORDEA statement csv";

      // ASSERT_TRUE(false) << "Make this test PASS";
    }

  }
}
