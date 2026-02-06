#pragma once

#include "csv/csv.hpp"                          // CSV::Table
#include "functional/maybe.hpp"                 // AnnotatedMaybe<T>...
#include "fiscal/amount/AccountStatement.hpp"   // AccountID, DomainPrefixedId
#include "text/functional.hpp"                  // contains_any_keyword,...
#include "logger/log.hpp"                       // logger::...
#include <optional>
#include <string>
#include <string_view>
#include <algorithm>
#include <ranges>
#include <regex>
#include <set>
#include <cctype> // std::isalnul,...

namespace account {
  namespace statement {

    namespace to_deprecate {
      namespace NORDEA {

        inline std::optional<std::size_t> to_avsandare_column_index(CSV::Table::Heading const& heading) {
          std::initializer_list<std::string_view> avsandare_keywords = {
            "avsandare", "avsändare"
          };

          for (std::size_t i = 0; i < heading.size(); ++i) {
            if (text::functional::contains_any_keyword(heading[i], avsandare_keywords)) {
              return i;
            }
          }
          return std::nullopt;
        }

        inline bool is_account_statement_table(CSV::Table const& table) {
          auto heading = table.heading;

          if (heading.size() == 0) {
            return false;
          }

          // Count how many NORDEA-specific keywords are found in the header
          int nordea_keyword_count = 0;
          std::initializer_list<std::string_view> nordea_keywords = {
            "bokforingsdag", "bokföringsdag",  // transaction date
            "avsandare", "avsändare",          // sender
            "mottagare",                        // receiver
            "saldo",                            // balance
            "belopp"                            // amount
          };

          for (auto const& col : heading) {
            if (text::functional::contains_any_keyword(col, nordea_keywords)) {
              ++nordea_keyword_count;
            }
          }

          // If we find at least 2 NORDEA-specific keywords, it's likely a NORDEA CSV
          return nordea_keyword_count >= 2;
        } // is_account_statement_table

        /**
        * TODO: This function is Claude generated and probaly flawed?
        *       I imagine the 'avsändare' may also conrtain foreign account numbers for
        *       transactions TO the bank account of which the account stement is about?
        */
        inline std::optional<std::string> to_account_no(CSV::Table const& table) {
          auto maybe_col_idx = to_avsandare_column_index(table.heading);
          if (!maybe_col_idx) {
            return {};
          }

          std::size_t col_idx = *maybe_col_idx;

          // Skip the first row if it's the header (rows[0] == heading in current implementation)
          // Look for the first non-empty value in the Avsandare column
          for (auto const& row : table.rows) {
            if (col_idx < row.size()) {
              std::string const& cell_value = row[col_idx];
              // Skip empty cells and the header cell
              if (!cell_value.empty() && !text::functional::contains_any_keyword(cell_value, {"avsandare", "avsändare"})) {
                return cell_value;
              }
            }
          }

          return {};
        } // to_account_no

      } // NORDEA

      namespace SKV {

        inline bool is_account_statement_table(CSV::Table const& table) {
          std::initializer_list<std::string_view> skv_keywords = {
            "ingaende saldo", "ingående saldo", "ing saldo",
            "utgaende saldo", "utgående saldo", "utg saldo",
            "obetalt",
            "intaktsranta", "intäktsränta",
            "moms"
          };

          // Check heading first
          for (auto const& col : table.heading) {
            if (text::functional::contains_any_keyword(col, skv_keywords)) {
              return true;
            }
          }

          // Check content rows
          for (auto const& row : table.rows) {
            for (auto const& cell : row) {
              if (text::functional::contains_any_keyword(cell, skv_keywords)) {
                return true;
              }
            }
          }

          return false;
        } // is_account_statement_table

        inline std::optional<std::string> to_account_no(CSV::Table const& table) {
          // Regex patterns for organisation numbers
          // Pattern 1: 6 digits, optional dash, 4 digits (Swedish org number format)
          std::regex org_number_pattern(R"((\d{6})-?(\d{4}))");
          // Pattern 2: SK followed by 10 digits
          std::regex sk_pattern(R"(SK(\d{10}))");

          auto search_text = [&](std::string const& text) -> std::optional<std::string> {
            std::smatch match;

            // Try SK pattern first (more specific)
            if (std::regex_search(text, match, sk_pattern)) {
              return match[1].str();  // Return just the digits
            }

            // Try org number pattern
            if (std::regex_search(text, match, org_number_pattern)) {
              // Return without dash: first 6 digits + last 4 digits
              return match[1].str() + match[2].str();
            }

            return std::nullopt;
          };

          // Search heading first
          for (auto const& col : table.heading) {
            if (auto org = search_text(col)) {
              return org;
            }
          }

          // Search all cells in all rows
          for (auto const& row : table.rows) {
            for (auto const& cell : row) {
              if (auto org = search_text(cell)) {
                return org;
              }
            }
          }

          return std::nullopt;
        } // to_account_no

      } // SKV

      namespace generic {

        enum class ColumnType {
          Undefined
          ,Empty
          ,Text
          ,Amount
          ,Date
          ,Unknown
        }; // ColumnType

        inline std::string to_string(ColumnType column_type) {
          std::string result{"??ColumnType??"};
          switch (column_type) {
            case ColumnType::Undefined: result = "Undefined"; break;
            case ColumnType::Empty: result = "Empty"; break;
            case ColumnType::Text: result = "Text"; break;
            case ColumnType::Amount: result = "Amount"; break;
            case ColumnType::Date: result = "Date"; break;
            case ColumnType::Unknown: result = "Unknown"; break;
          }
          return result;
        }

        inline ColumnType to_column_type(std::string field) {
          logger::scope_logger log_riia(
            logger::development_trace
            ,"to_column_type(field)"
            ,logger::LogToConsole::ON);

          ColumnType candidate{};
          // TODO: Design a way that works for UTF-8 
          //      (below covers ASCII = probably OK for western languages)
          //      Note that an unlucky UTF-8 text like 'åäöÅÄÖ' will be classified as Unknown.
          //      Single byte cgaracter sets e.g., ISO8859-1 may be fine though?
          //      It all comes down to how C++ runtime 'locale' implements std::isalnum etc...?
          if (
              (field.size() == 0) 
            or (std::ranges::fold_left(field,size_t{0},[](size_t acc,auto ch){
                  return acc + (std::isspace(ch)?0:1);
                }) == 0)) candidate = ColumnType::Empty;
          else if (auto maybe_date = to_date(field)) candidate = ColumnType::Date;
          else if (auto maybe_amount = to_amount(field)) candidate = ColumnType::Amount;
          else if (field.size() > 0 and std::ranges::all_of(field,[](auto ch){
            return std::isalnum(ch);
          })) candidate = ColumnType::Text;
          else if (field.size() > 8 and std::ranges::fold_left(field,size_t{0},[](size_t acc,auto ch){
            return acc + (std::isalnum(ch)?1:0);
          }) > field.size() / 2) candidate = ColumnType::Text; // more than half ASCII alpha numeric
          else candidate = ColumnType::Unknown;

          logger::development_trace("field:'{}' -> type:{}",field,to_string(candidate));
          return candidate;
        }

        using ColumnMap = std::map<int,ColumnType>;

        enum class EntryType {
          Undefined
          ,Emtpy
          ,Caption
          ,Heading
          ,Transaction
          ,Balance
          ,Unknown
        };

        inline std::string to_string(EntryType entry_type) {
          std::string result{"??EntryType??"};
          switch (entry_type) {
            case EntryType::Undefined: result = "Undefined"; break;
            case EntryType::Emtpy: result = "Emtpy"; break;
            case EntryType::Caption: result = "Caption"; break;
            case EntryType::Heading: result = "Heading"; break;
            case EntryType::Transaction: result = "Transaction"; break;
            case EntryType::Balance: result = "Balance"; break;
            case EntryType::Unknown: result = "Unknown"; break;
          }

          return result;
        }

        using EntryMap = MetaDefacto<EntryType,ColumnMap>;
        using EntryMaps = std::vector<EntryMap>;

        inline std::string to_string(EntryMap const& entry_map) {
          std::string result{"??EntryMap??"};
          return result;
        }

        inline EntryType to_entry_type(EntryMap const& entry_map) {
          logger::scope_logger log_riia(
            logger::development_trace
            ,"to_entry_map(row)"
            ,logger::LogToConsole::ON);

          EntryType candidate{};
          logger::development_trace(
            "entry_map:'{}' -> type:{}"
            ,to_string(entry_map)
            ,to_string(candidate));
          return candidate;
        }

        inline EntryMap to_entry_map(CSV::Table::Row const& row) {
          logger::scope_logger log_riia(
          logger::development_trace
          ,"to_entry_map(row)"
          ,logger::LogToConsole::ON);

          ColumnMap column_map{};
          for (int i=0;i<row.size();++i) {
            column_map[i] = to_column_type(row[i]);
          }

          EntryType entry_type{};
          std::map<ColumnType,int> type_count{};
          for (auto const& entry : column_map) {
            ++type_count[entry.second];
          }

          if (type_count[ColumnType::Amount] == 2) {
            // Determine if we have trans + saldo amounts in the entry

          }
                  
          EntryMap candidate{};

          logger::development_trace(
            "row:'{}' -> map:{}"
            ,to_string(row)
            ,to_string(candidate));

          candidate = {.meta = entry_type,.defacto = column_map};

          return candidate;
        }

        inline std::optional<CSV::MDTable<EntryMaps>> to_entries_mapped(CSV::Table const& table) {

          logger::scope_logger log_riia(
            logger::development_trace
            ,"to_entries_mapped(csv)"
            ,logger::LogToConsole::ON);

          // Identify transaction entries

          // map candidates
          std::vector<EntryMap> candidates{};
          for (auto const& entry : table.rows) {
            candidates.push_back(to_entry_map(entry));
          }

          for (int i=0;i<table.rows.size();++i) {
            logger::development_trace(
              "{} type:{}"
              ,table.rows[i].to_string()
              ,"not yet implemented");
          }

          // Find the entry field count ditsribution (table with uniform entry formats?)
          std::set<size_t> col_counts{};
          for (auto const& entry : table.rows) {
            col_counts.insert(entry.size());
          }

          if (col_counts.size() == 1) {
            // All entries formatted as columns
          }
          else {
            // Can we still interpret this table as having at least one transaction entry?
          }

          return {}; // failed
        }

      } // generic

      namespace maybe {

        inline std::optional<CSV::MDTable<generic::EntryMaps>> to_entries_mapped(CSV::Table const& table) {
          return generic::to_entries_mapped(table);
        }

      } // maybe

    } // to_deprecate

    namespace maybe {

      std::optional<CSV::MDTable<account::statement::TableMeta>> to_statement_id_ed_step(CSV::Table const& table);

    } // maybe

    namespace monadic {
      AnnotatedMaybe<CSV::MDTable<account::statement::TableMeta>> to_statement_id_ed_step(CSV::Table const& table);

    } // monadic

  } // statement
} // account
