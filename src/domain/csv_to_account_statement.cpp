#include "csv_to_account_statement.hpp"
#include "text/functional.hpp" // functional::text::filtered

namespace account {

  namespace statement {

    namespace maybe {

      namespace table {

        std::string to_string(FieldType field_type) {
          std::string result{"?FieldType?"};
          switch (field_type) {
           case FieldType::Unknown: result = "Unknown"; break;
            case FieldType::Empty: result = "Empty"; break;
            case FieldType::Date: result = "Date"; break;
            case FieldType::Amount: result = "Amount"; break;
            case FieldType::Text: result = "Text"; break;
            case FieldType::Undefined: result = "Undefined"; break;
            default: result = std::format("?FieldType:{}?",static_cast<int>(field_type)); break;
          }
          return result;
        }

        FieldType to_field_type(std::string const& s) {
          if (s.size()==0) {
            return FieldType::Empty;
          }
          else if (auto maybe_date = to_date(s)) {
            return FieldType::Date;
          }
          else if (auto maybe_amount = to_amount(s)) {
            return FieldType::Amount;
          }
          else if (text::functional::count_alpha(s) > 0) {
            return FieldType::Text;
          }
          return FieldType::Unknown;
        }

        RowMap to_row_map(CSV::Table::Row const& row) {
          RowMap result{};

          for (unsigned i=0;i<row.size();++i) {
            result.ixs[to_field_type(row[i])].push_back(i);
          }

          return result;
        }

        RowsMap to_rows_map(CSV::Table::Rows const& rows) {
          RowsMap result{};

          for (auto const& row : rows) {
            result.push_back(to_row_map(row));
          }

          return result;
        }

        ColumnMapping detect_columns_from_data(CSV::Table::Rows const& rows) {
          ColumnMapping mapping;

          logger::scope_logger log_raii(logger::development_trace,"detect_columns_from_data");

          if (true) {
            // Refactored to use per row RowsMap analysis

            auto rows_map = to_rows_map(rows);

            if (true) {
              // Log
              for (size_t i=0;i<rows.size();++i) {
                std::string map_string{};

                for (auto const& entry : rows_map[i].ixs) {
                  map_string += std::format(" {}",to_string(entry.first));
                  map_string += ":";
                  for (auto ix : entry.second) {
                    map_string += std::format(" {}",ix);
                  }
                }

                logger::development_trace(
                   "row:{} map:{}"
                  ,rows[i].to_string()
                  ,map_string);
              }

              // Hard coded for SKV account statement file
              {
                auto is_skv_saldo_entry_candidate = [](auto const& row_map){
                    if (!row_map.ixs.contains(FieldType::Date) or row_map.ixs.at(FieldType::Date).size()!=1) return false;
                    if (!row_map.ixs.contains(FieldType::Amount) or row_map.ixs.at(FieldType::Amount).size()!=1) return false;
                    return true;
                  };
                auto in_saldo_candidate_iter = std::ranges::find_if(
                  rows_map
                  ,is_skv_saldo_entry_candidate
                );

                if (in_saldo_candidate_iter != rows_map.end()) {
                  auto trans_span_begin = in_saldo_candidate_iter+1;
                  auto out_saldo_candidate_iter = std::find_if(
                     trans_span_begin
                    ,rows_map.end()
                    ,is_skv_saldo_entry_candidate
                  );

                  if (out_saldo_candidate_iter != rows_map.end()) {
                    auto trans_span_end = out_saldo_candidate_iter;
                    // We have a span to check
                    auto key_map = *(in_saldo_candidate_iter+1);
                    auto is_skv_trans_entry_candidate = [&key_map](auto const& row_map){
                      if (!row_map.ixs.contains(FieldType::Date) or row_map.ixs.at(FieldType::Date).size()!=1) return false;
                      if (!row_map.ixs.contains(FieldType::Text) or row_map.ixs.at(FieldType::Text).size()!=1) return false;
                      if (!row_map.ixs.contains(FieldType::Amount) or row_map.ixs.at(FieldType::Amount).size()!=2) return false;
                      return row_map.ixs == key_map.ixs;
                    };

                    if (std::all_of(
                       trans_span_begin
                      ,trans_span_end
                      ,is_skv_trans_entry_candidate
                    )) {
                      // Good enough - This is probably an SKV account statement csv file table
                      mapping.date_column = key_map.ixs.at(FieldType::Date).front();
                      mapping.description_column = key_map.ixs.at(FieldType::Text).front();
                      mapping.transaction_amount_column = key_map.ixs.at(FieldType::Amount)[0];
                      mapping.saldo_amount_column = key_map.ixs.at(FieldType::Amount)[1];
                      return mapping;
                    }
                  }

                }

              }


              std::print("\ndetect_columns_from_data: TODO - ISSUE20260114_SKV_CSV Create ColumnMapping from rows_map");
              
            } // true (Log)


          }

          if (rows.empty()) {
            return mapping;
          }

          // Determine number of columns
          size_t num_columns = 0;
          for (auto const& row : rows) {
            num_columns = std::max(num_columns, row.size());
          }

          if (num_columns == 0) {
            return mapping;
          }

          // Sample rows for analysis (skip empty first columns - balance rows)
          auto sample_rows = 
              rows
            | std::views::filter([](auto const& row) {
                return row.size() > 0 && row[0].size() > 0;
              })
            | std::views::take(10);

          std::vector<CSV::Table::Row> sampled;
          std::ranges::copy(sample_rows, std::back_inserter(sampled));

          if (sampled.empty()) {
            logger::development_trace("if (sampled.empty()) -> returns mapping.is_valid:{}",mapping.is_valid());
            return mapping;
          }

          // Check each column
          for (size_t col = 0; col < num_columns; ++col) {

            int valid_dates = 0;
            int valid_amounts = 0;

            for (auto const& row : sampled) {
              if (col >= row.size() || row[col].size() == 0) {
                continue;
              }

              if (to_date(row[col]).has_value()) {
                valid_dates++;
              }
              else if (to_amount(row[col]).has_value()) {
                valid_amounts++;
              }
              else {
                // Nor date or amount
              }
            } // for row

            if (valid_dates == sampled.size()) {
              // Column is Date if ALL values are valid dates
              mapping.date_column = static_cast<int>(col);
            }
            else if (valid_amounts == sampled.size()) {
              // First amount column found becomes transaction amount
              // Second amount column found becomes saldo amount
              if (mapping.transaction_amount_column == -1) {
                mapping.transaction_amount_column = static_cast<int>(col);
              }
              else if (mapping.saldo_amount_column == -1) {
                mapping.saldo_amount_column = static_cast<int>(col);
              }
            }
            else {
              // Not a date nor an amount column
            }
          } // for column

          // Description is typically the first text column that's not date or amount
          for (size_t col = 0; col < num_columns; ++col) {
            if (static_cast<int>(col) != mapping.date_column &&
                static_cast<int>(col) != mapping.transaction_amount_column &&
                static_cast<int>(col) != mapping.saldo_amount_column &&
                mapping.description_column == -1) {

              // Check if column has substantial text content
              bool has_text = false;
              for (auto const& row : sampled) {
                if (col < row.size() && row[col].size() > 2) {
                  has_text = true;
                  break;
                }
              }

              if (has_text) {
                mapping.description_column = static_cast<int>(col);
              }
            }
          }

          logger::development_trace("returns mapping.is_valid:{}",mapping.is_valid());

          return mapping;
        } // detect_columns_from_data

      } // table
    } // maybe
  } // statement
} // account