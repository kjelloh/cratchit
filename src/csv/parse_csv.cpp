
#include "parse_csv.hpp"
#include "logger/log.hpp"
#include "std_overload.hpp" // std_overload::overload,...
#include <fstream>

namespace CSV {

  namespace parse {

    namespace maybe {

      std::optional<CSV::Table> csv_to_table(std::string_view csv_text, char delim) {
        logger::scope_logger log_raii{logger::development_trace, "CSV::parse::maybe::csv_to_table(string_view)"};
        // Handle empty input
        if (csv_text.empty()) {
          return std::nullopt;
        }

        // Parse all rows
        std::vector<std::vector<std::string>> all_rows;
        size_t pos = 0;

        while (pos < csv_text.size()) {
          // Skip empty lines
          while (pos < csv_text.size() && (csv_text[pos] == '\n' || csv_text[pos] == '\r')) {
            pos++;
          }

          if (pos >= csv_text.size()) {
            break;
          }

          auto row = csv_row_to_fields(csv_text, pos, delim);

          // Only add non-empty rows
          if (!row.empty()) {
            all_rows.push_back(std::move(row));
          }
        }

        // Need at least one row (header)
        if (all_rows.empty()) {
          return std::nullopt;
        }

        // Create Table structure
        CSV::Table table;

        // First row is the heading
        // Convert vector<string> to Key::Path (which is what FieldRow/Heading is)
        table.heading = Key::Path{all_rows[0]};

        // Convert remaining rows
        for (size_t i = 0; i < all_rows.size(); ++i) {
          table.rows.push_back(Key::Path{all_rows[i]});
        }

        logger::development_trace("Returns table with size:{}",table.rows.size());
        return table;
      } // csv_to_table

      std::optional<CSV::Table> csv_text_to_table_step(std::string_view csv_text) {
        return csv_to_table(csv_text,to_csv_delimiter(csv_text));
      } // csv_text_to_table_step

    } // maybe

    namespace monadic {

      AnnotatedMaybe<CSV::Table> csv_text_to_table_step(std::string_view csv_text) {

        auto to_msg = [](CSV::Table const& result){
          return std::format("{} rows",result.rows.size());
        };

        auto f =  cratchit::functional::to_annotated_maybe_f(
          CSV::parse::maybe::csv_text_to_table_step
          ,"csv table"
          ,to_msg);

        return f(csv_text);

      }

    }

  } // parse


} // CSV