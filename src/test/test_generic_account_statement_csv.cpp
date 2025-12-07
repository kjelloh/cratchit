#include "logger/log.hpp"
#include "csv/parse_csv.hpp"
#include "csv/csv_to_account_id.hpp"
#include "test/data/account_statements_csv.hpp"
#include <gtest/gtest.h>

namespace tests::generic_statement_csv {

  namespace happy_path_suite  {

    TEST(GenericStatementCSVFieldsTests,FieldTypesOk) {
      {
        std::vector<std::string> empty_fields{
          ""
          ," "
          ,"\t"
          ,"\t "
          ,"\n"
          ,"\n "
          ,"          "
          ,"\t        "
          ,"\n        "
        };
        namespace asg = account::statement::generic;
        for (auto const& field : empty_fields) {
          auto column_type = asg::to_column_type(field);
          ASSERT_EQ(column_type,asg::ColumnType::Empty)
            << std::format(
                  "Expected '{}' to be validated as Empty but encountered {}"
                  ,field
                  ,asg::to_string(column_type));
        }
      }
      {
        std::vector<std::string> date_fields{
           "20251207"
          ,"2025-12-07"
          ,"2025/12/07"

          ," 20251207"
          ," 2025-12-07"
          ," 2025/12/07"

          ," 20251207 "
          ," 2025-12-07 "
          ," 2025/12/07 "

          ,"20251207         "
          ,"2025-12-07       "
          ,"2025/12/07       "
        };
        namespace asg = account::statement::generic;
        for (auto const& field : date_fields) {
          auto column_type = asg::to_column_type(field);
          ASSERT_EQ(column_type,asg::ColumnType::Date)
            << std::format(
                  "Expected '{}' to be validated as Date but encountered {}"
                  ,field
                  ,asg::to_string(column_type));
        }
      }

      {
        std::vector<std::string> amount_fields{
           "123,00"
          ,"123.00"
          ,"123"
          ,"12"
          ,"1"
          ,"0,00"
          ,"0.00"
          ,"0,75"
          ,"0.75"          
        };
        namespace asg = account::statement::generic;
        for (auto const& field : amount_fields) {
          auto column_type = asg::to_column_type(field);
          ASSERT_EQ(column_type,asg::ColumnType::Amount)
            << std::format(
                  "Expected '{}' to be validated as Amount but encountered {}"
                  ,field
                  ,asg::to_string(column_type));
        }
      }

      {
        std::vector<std::string> text_fields{
           "Belopp"
          ,"Faktura"
          ,"Fak"
          ,"F"
          ,"Bolaget.org"
          ,"Å ena sidan - Å andra sidan"
          ,"åäabcdef" // UTF-8 åä is escaped to 2*3 bytes = 6 bytes 'abcdef' -> 1/2 std::isalnum ok
          ,"åäabcde"
          // ,"åäabcd" // UTF-8 åä is escaped to 2*3 = 6 bytes overwhelms 4 bytes std::isalnum
          // ,"åäöÅÄÖ" // Fails for now.
          //              TODO: Add test when cratchit is fully 'runtime encoding' aware
        };
        namespace asg = account::statement::generic;
        for (auto const& field : text_fields) {
          auto column_type = asg::to_column_type(field);
          ASSERT_EQ(column_type,asg::ColumnType::Text)
            << std::format(
                  "Expected '{}' to be validated as Text but encountered {}"
                  ,field
                  ,asg::to_string(column_type));
        }
      }

    }

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
