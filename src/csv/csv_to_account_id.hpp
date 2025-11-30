#pragma once

#include "csv/csv.hpp"                          // CSV::Table
#include "fiscal/amount/AccountStatement.hpp"   // AccountID, DomainPrefixedId
#include "text/functional.hpp" // contains_any_keyword,...
#include "logger/log.hpp"                       // logger::...
#include <optional>
#include <string>
#include <string_view>
#include <algorithm>
#include <ranges>
#include <regex>

namespace account {
  namespace statement {

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

      /**
      * Check if the CSV::Table heading indicates a NORDEA format
      *
      * NORDEA headers contain specific Swedish banking column names:
      * - "Bokforingsdag" (transaction date)
      * - "Avsandare" (sender - contains account number)
      * - "Mottagare" (receiver)
      * - "Saldo" (balance)
      */
      inline bool is_nordea_account_statement_table(CSV::Table const& table) {
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
      } // is_nordea_account_statement_table

      /**
      * TODO: This function is Claude generated and probaly flawed?
      *       I imagine the 'avsändare' may also conrtain foreign account numbers for
      *       transactions TO the bank account of which the account stement is about?
      */
      inline std::optional<std::string> to_nordea_account_id(CSV::Table const& table) {
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
      } // to_nordea_account_id

    } // NORDEA

    namespace SKV {

      /**
      * Check if the CSV::Table content indicates an SKV (tax agency) format
      *
      * SKV CSV files have specific patterns:
      * - Headers or content containing: "Ingaende saldo", "Utgaende saldo", "Obetalt"
      * - Organisation number format: 10 digits like "5567828172" or "556782-8172"
      * - The newer format (sz_SKV_csv_20251120) has company name and org number in first row
      */
      inline bool is_skv_account_statement_table(CSV::Table const& table) {
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
      } // is_skv_account_statement_table

      /**
      * Search for a 10-digit organisation number in SKV data
      *
      * Organisation numbers can appear as:
      * - Pure 10 digits: "5567828172"
      * - With dash: "556782-8172"
      * - In text like "Inbetalning fran organisationsnummer XXXXXXXXXX"
      * - In SK reference: "SK5567828172"
      */
      inline std::optional<std::string> to_skv_org_number(CSV::Table const& table) {
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
      } // to_skv_org_number

    } // SKV

    namespace maybe {

      // Monadic Maybe: Table -> (account ID,Table) pair
      inline std::optional<CSV::MDTable<AccountID>> to_account_id_ed(CSV::Table const& table) {

        logger::development_trace("to_account_id_ed: Starting AccountID extraction");

        // Check for fundamentally invalid table
        if (table.heading.size() == 0 && table.rows.empty()) {
          logger::development_trace("to_account_id_ed: Empty table, returning nullopt");
          return std::nullopt;
        }

        // Step 1: Check for NORDEA format
        if (account::statement::NORDEA::is_nordea_account_statement_table(table)) {
          std::string account_number = account::statement::NORDEA::to_nordea_account_id(table).value_or("");
          logger::development_trace("to_account_id_ed: Detected NORDEA CSV, account: '{}'", account_number);
          return CSV::MDTable<AccountID>{AccountID{"NORDEA", account_number}, table};
        }

        // Step 2: Check for SKV format
        if (account::statement::SKV::is_skv_account_statement_table(table)) {
          auto maybe_org_number = account::statement::SKV::to_skv_org_number(table);
          std::string org_number = maybe_org_number.value_or("");
          logger::development_trace("to_account_id_ed: Detected SKV CSV, org number: '{}'", org_number);
          return CSV::MDTable<AccountID>{AccountID{"SKV", org_number}, table};
        }

        // Step 3: Unknown format - fully unknown AccountID, return nullopt (failure)
        logger::development_trace("to_account_id_ed: Unknown Account Statement format, returning nullopt");
        return std::nullopt;
      }

    } // maybe

  } // statement
} // account
