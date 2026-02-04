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

        std::string to_string(RowMap const& row_map) {
          std::string result{};

          for (auto const& entry : row_map.ixs) {
            result += std::format(" {}",to_string(entry.first));
            result += ":";
            for (auto ix : entry.second) {
              result += std::format(" {}",ix);
            }
          }
          return result;
        }

        void log_the_rows_map(CSV::Table::Rows const& rows,RowsMap const& rows_map) {
          for (size_t i=0;i<rows.size();++i) {
            logger::development_trace(
              "\n\trow:{} \n\tmap:{}"
              ,rows[i].to_string()
              ,to_string(rows_map[i]));
          } // for rows

        } // log_the_rows_map

        ColumnMapping skv_like_to_column_mapping(CSV::Table const& table) {
          logger::scope_logger log_raii(logger::development_trace,"skv_like_to_column_mapping");

          ColumnMapping result{};
          auto rows_map = to_rows_map(table.rows);
          // Try for SKV account statement file

          if (true) log_the_rows_map(table.rows,rows_map);

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
                result.date_column = key_map.ixs.at(FieldType::Date).front();
                result.description_column = key_map.ixs.at(FieldType::Text).front();
                result.transaction_amount_column = key_map.ixs.at(FieldType::Amount)[0];
                result.saldo_amount_column = key_map.ixs.at(FieldType::Amount)[1];
              }
            }

          }

          if (true) logger::development_trace("returns result.is_valid:{}",result.is_valid());

          return result;
        } // skv_like_to_column_mapping

        ColumnMapping nordea_like_to_column_mapping(CSV::Table const& table) {
          logger::scope_logger log_raii(logger::development_trace,"nordea_like_to_column_mapping",logger::LogToConsole::ON);

          ColumnMapping result{};

          auto const& rows = table.rows;
          auto rows_map = to_rows_map(rows);
          if (true) log_the_rows_map(table.rows,rows_map);

          // Try for NORDEA like account statement table
          logger::development_trace("rows_map size {}",rows_map.size());

          if (rows_map.size() == 0) return {}; // Empty table

          auto is_amount_and_saldo_entry_candidate = [](auto const& row_map){
              if (!row_map.ixs.contains(FieldType::Date) or row_map.ixs.at(FieldType::Date).size()!=1) return false;
              if (!row_map.ixs.contains(FieldType::Amount) or row_map.ixs.at(FieldType::Amount).size()<2) return false;
              if (!row_map.ixs.contains(FieldType::Text) or row_map.ixs.at(FieldType::Text).size()==0) return false;
              return true;
            };

          auto first_trans_iter_candidate = std::ranges::find_if(
             rows_map
            ,is_amount_and_saldo_entry_candidate);

          if (first_trans_iter_candidate == rows_map.end()) {
            logger::development_trace("No is_amount_and_saldo_entry_candidate match");
          }

          auto trans_iter_candidates_end = std::find_if_not(
             first_trans_iter_candidate
            ,rows_map.end()
            ,is_amount_and_saldo_entry_candidate
          );

          auto trans_candidates_count = std::distance(first_trans_iter_candidate,trans_iter_candidates_end);
          if (true) logger::development_trace("trans_candidates_count:{}",trans_candidates_count);

          // Fold all candidate row maps using 'common' to get the common row map
          auto iter = first_trans_iter_candidate +1;
          auto iter_end = trans_iter_candidates_end;
          auto common = *first_trans_iter_candidate;
          while (iter != iter_end) {
            auto const& rhs = *iter;
            RowMap next{};
            for (auto const& [lhs_type,lhs_v] : common.ixs) {
              if (!rhs.ixs.contains(lhs_type)) continue;
              auto const& rhs_v = rhs.ixs.at(lhs_type);
              std::vector<FieldIx> intersection_v{};
              std::ranges::set_intersection(
                 lhs_v
                ,rhs_v
                ,std::back_inserter(intersection_v)
              );
              if (intersection_v.size()>0) next.ixs[lhs_type] = intersection_v;
            }
            common = next;
            ++iter;
          }

          if (true) logger::development_trace("common :{}",to_string(common));

          if (!common.ixs.contains(FieldType::Date) or common.ixs.at(FieldType::Date).size() != 1) {
            if (true) logger::development_trace("Expected common Date column");
            return {};
          }
          if (!common.ixs.contains(FieldType::Amount) or common.ixs.at(FieldType::Amount).size() != 2) {
            if (true) logger::development_trace("Expected two common amount columns");
            return {};
          }
          if (!common.ixs.contains(FieldType::Text) or common.ixs.at(FieldType::Text).size() == 0) {
            if (true) logger::development_trace("Expected at least one common text column");
            return {};
          }

          auto begin_rix = std::distance(rows_map.begin(),first_trans_iter_candidate);
          auto end_rix = std::distance(rows_map.begin(),trans_iter_candidates_end);

          std::vector<std::pair<Amount,Amount>> amounts{};
          for (auto rix=begin_rix;rix<end_rix;++rix) {
            auto const& row = rows[rix];
            auto first_cix = common.ixs.at(FieldType::Amount).front();
            auto second_cix = common.ixs.at(FieldType::Amount).back();
            auto maybe_first_amount = to_amount(row[first_cix]);
            auto maybe_second_amount = to_amount(row[second_cix]);
            if (maybe_first_amount and maybe_second_amount) {
              amounts.push_back({*maybe_first_amount,*maybe_second_amount});
            }
            else {
              if (true) logger::design_insufficiency("Expected two valid amount after successfull rows mapping");
              return {};
            }
          }

          if (amounts.size()<2) {
            if (true) logger::design_insufficiency("nordea_like_to_column_mapping - Failed to determine saldo amount column for less than two entry candidates");
            return {};
          }

          unsigned first_trans_second_saldo_count{};
          unsigned first_saldo_second_trans_count{};
          unsigned undetermined_amounts_count{};
          // Assume transactions are in revrese new-to-old order
          for (size_t earlier_six=amounts.size()-1;earlier_six > 0;--earlier_six) {
            auto later_six = earlier_six-1;
            auto [earlier_first,earlier_second] = amounts[earlier_six];
            auto [later_first,later_second] = amounts[later_six];

            logger::development_trace(
               "earlier_first:{} earlier_second:{} later_first:{} later_second:{}"
              ,::to_string(earlier_first)
              ,::to_string(earlier_second)
              ,::to_string(later_first)
              ,::to_string(later_second));
            
            logger::development_trace(
               "earlier_second:{} + later_first:{} = {} ==?== {} : later_second"
               ,::to_string(earlier_second)
               ,::to_string(later_first)
               ,::to_string(earlier_second+later_first)
               ,::to_string(later_second));

            if (earlier_second+later_first == later_second) {
              ++first_trans_second_saldo_count;
              logger::development_trace("first_trans_second_saldo_count:{}",first_trans_second_saldo_count);
            }
            else if (earlier_first+later_second == later_first) {
              ++first_saldo_second_trans_count;
              logger::development_trace("first_saldo_second_trans_count:{}",first_saldo_second_trans_count);
            }
            else {
              ++undetermined_amounts_count;
              logger::development_trace("undetermined_amounts_count:{}",undetermined_amounts_count);
            }

          }

          if (true) logger::development_trace(
             "first_trans_second_saldo_count:{} first_saldo_second_trans_count:{} undetermined_amounts_count:{}"
            ,first_trans_second_saldo_count
            ,first_saldo_second_trans_count
            ,undetermined_amounts_count);

          if (true) logger::development_trace("returns result.is_valid:{}",result.is_valid());

          return result;
        } // nordea_like_to_column_mapping

        namespace to_deprecate {
          ColumnMapping detect_columns_from_header(CSV::Table::Heading const& heading) {

            logger::scope_logger log_raii(logger::development_trace,"detect_columns_from_header");

            ColumnMapping mapping;

            // Helper to check if a string contains a keyword (case-insensitive)
            auto contains_keyword = [](std::string_view text, std::vector<std::string_view> const& keywords) {
              std::string lower_text;
              lower_text.reserve(text.size());
              std::ranges::transform(text, std::back_inserter(lower_text),
                [](char c) { return std::tolower(c); });

              return std::ranges::any_of(keywords, [&lower_text](std::string_view keyword) {
                return lower_text.find(keyword) != std::string::npos;
              });
            };

            // Date keywords
            std::vector<std::string_view> date_keywords = {
              "date", "datum", "bokföringsdag", "dag", "bokforingsdag"
            };

            // Amount keywords
            std::vector<std::string_view> amount_keywords = {
              "amount", "belopp", "suma", "summa"
            };

            // Description keywords (prioritized)
            std::vector<std::string_view> primary_description_keywords = {
              "namn", "name", "description", "rubrik", "titel", "title"
            };

            std::vector<std::string_view> secondary_description_keywords = {
              "meddelande", "message", "text", "detaljer", "details"
            };

            // Scan header columns
            for (size_t i = 0; i < heading.size(); ++i) {
              std::string col_name = heading[i];

              // Check for date column
              if (mapping.date_column == -1 && contains_keyword(col_name, date_keywords)) {
                mapping.date_column = static_cast<int>(i);
              }

              // Check for transaction amount column
              if (mapping.transaction_amount_column == -1 && contains_keyword(col_name, amount_keywords)) {
                mapping.transaction_amount_column = static_cast<int>(i);
              }

              // Check for primary description column
              if (mapping.description_column == -1 && contains_keyword(col_name, primary_description_keywords)) {
                mapping.description_column = static_cast<int>(i);
              }

              // Check for additional description columns
              if (contains_keyword(col_name, secondary_description_keywords)) {
                mapping.additional_description_columns.push_back(static_cast<int>(i));
              }
            }

            logger::development_trace("returns mapping.is_valid:{}",mapping.is_valid());

            return mapping;
          } // detect_columns_from_header

          ColumnMapping detect_columns_from_data(CSV::Table::Rows const& rows) {
            ColumnMapping mapping;

            logger::scope_logger log_raii(logger::development_trace,"detect_columns_from_data");

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

          ColumnMapping to_column_mapping(CSV::Table const& table) {
            ColumnMapping result{};
            result = table::to_deprecate::detect_columns_from_header(table.heading);
            if (!result.is_valid()) {
              result = table::to_deprecate::detect_columns_from_data(table.rows);
            }
            return result;
          } // to_column_mapping

        } // to_deprecate

        namespace detail {

          using ColumnMappingFn = ColumnMapping(*)(CSV::Table const&);

          constexpr std::array<ColumnMappingFn, 3> column_mapping_projectors = {
             &table::skv_like_to_column_mapping
            ,&table::nordea_like_to_column_mapping
            ,&table::to_deprecate::to_column_mapping
          }; 


        } // detail

        ColumnMapping to_column_mapping(CSV::Table const& table) {
            for (auto project : detail::column_mapping_projectors) {
                ColumnMapping mapping = project(table);
                if (mapping.is_valid()) {
                    return mapping;
                }
            }
            return {};
        } // to_column_mapping

        TableMeta to_account_statement_table_meta(CSV::Table const& table) {
          TableMeta result{};
          result.trans_row_mapping = to_column_mapping(table);
          return result;
        } // to_account_statement_table_meta


        bool is_ignorable_row(CSV::Table::Row const& row, ColumnMapping const& mapping) {
          // Empty row is ignorable
          if (row.size() == 0) {
            return true;
          }

          // Row with empty first column (common pattern for balance rows)
          if (row[0].size() == 0) {
            return true;
          }

          // Check description column for balance keywords
          if (mapping.description_column >= 0 &&
              static_cast<size_t>(mapping.description_column) < row.size()) {

            std::string desc = row[mapping.description_column];
            std::ranges::transform(desc, desc.begin(),
              [](char c) { return std::tolower(c); });

            if (desc.find("saldo") != std::string::npos ||
                desc.find("ingående") != std::string::npos ||
                desc.find("ingaende") != std::string::npos ||
                desc.find("utgående") != std::string::npos ||
                desc.find("utgaende") != std::string::npos) {
              return true;
            }
          }

          std::print("\nis_ignorable_row: TODO - ISSUE20260114_SKV_CSV Ignore row if mapping fails");

          return false;
        } // is_ignorable_row

      } // table

      OptionalAccountStatementEntries csv_table_to_account_statement_entries(CSV::Table const& table) {
        logger::scope_logger log_raii{logger::development_trace, "csv_table_to_account_statement_entries(table)"};
        
        auto mapping = table::to_column_mapping(table);

        if (!mapping.is_valid()) {
          return std::nullopt;
        }

        // Extract entries from rows
        AccountStatementEntries entries;

        for (auto const& row : table.rows) {
          auto maybe_entry = extract_entry_from_row(row, mapping);
          if (maybe_entry) {
            entries.push_back(*maybe_entry);
          }
        }

        logger::development_trace("Returns entries with size:{}",entries.size());

        // Return the entries (may be empty if all rows were ignorable)
        return entries;
      } // csv_table_to_account_statement_entries

      std::optional<AccountStatement> csv_table_to_account_statement_step(
          CSV::Table const& table,
          AccountID const& account_id) {
        logger::scope_logger log_raii{logger::development_trace, "csv_table_to_account_statement_step(table, account_id)"};

        // Extract entries from the CSV table
        auto maybe_entries = csv_table_to_account_statement_entries(table);

        if (!maybe_entries) {
          logger::development_trace("Failed to extract entries from CSV table");
          return std::nullopt;
        }

        // Create AccountStatement with entries and account ID metadata
        AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};

        logger::development_trace("Creating AccountStatement with {} entries and account_id: {}",
          maybe_entries->size(), account_id.to_string());

        return AccountStatement(*maybe_entries, meta);
      } // csv_table_to_account_statement_step

      std::optional<AccountStatement> account_id_ed_to_account_statement_step(CSV::MDTable<AccountID> const& account_id_ed) {
        logger::scope_logger log_raii{logger::development_trace,
          "account_id_ed_to_account_statement_step(account_id_ed)"};

        // Extract components from MDTable
        CSV::Table const& table = account_id_ed.defacto;
        AccountID const& account_id = account_id_ed.meta;

        logger::development_trace("Processing MDTable with AccountID: '{}'", account_id.to_string());

        // Extract entries from the CSV table
        auto maybe_entries = csv_table_to_account_statement_entries(table);

        if (!maybe_entries) {
          logger::development_trace("Failed to extract entries from CSV table in MDTable");
          return std::nullopt;
        }

        // Create AccountStatement with entries and account ID metadata
        AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};

        logger::development_trace("Creating AccountStatement with {} entries and account_id: {}",
          maybe_entries->size(), account_id.to_string());

        return AccountStatement(*maybe_entries, meta);
      } // account_id_ed_to_account_statement_step


    } // maybe

    namespace monadic {

      AnnotatedMaybe<AccountStatement> account_id_ed_to_account_statement_step(CSV::MDTable<AccountID> const& account_id_ed) {

        auto f = cratchit::functional::to_annotated_maybe_f(
           account::statement::maybe::account_id_ed_to_account_statement_step
          ,"Account ID.ed table -> account statement failed");

        return f(account_id_ed);

      }


    } // monadic
  } // statement
} // account