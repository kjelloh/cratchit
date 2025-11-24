#pragma once

#include "csv/csv.hpp"                          // CSV::Table
#include "fiscal/amount/AccountStatement.hpp"   // AccountID, DomainPrefixedId
#include "logger/log.hpp"                       // logger::...
#include <optional>
#include <string>
#include <string_view>
#include <algorithm>
#include <ranges>
#include <regex>

namespace CSV {
  namespace project {

    namespace account_id_detection {

      /**
       * Helper to check if a string contains any of the given keywords (case-insensitive)
       */
      inline bool contains_any_keyword(std::string_view text, std::initializer_list<std::string_view> keywords) {
        std::string lower_text;
        lower_text.reserve(text.size());
        std::ranges::transform(text, std::back_inserter(lower_text),
          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        return std::ranges::any_of(keywords, [&lower_text](std::string_view keyword) {
          return lower_text.find(keyword) != std::string::npos;
        });
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
      inline bool is_nordea_csv(CSV::Table::Heading const& heading) {
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
          if (contains_any_keyword(col, nordea_keywords)) {
            ++nordea_keyword_count;
          }
        }

        // If we find at least 2 NORDEA-specific keywords, it's likely a NORDEA CSV
        return nordea_keyword_count >= 2;
      }

      /**
       * Find the index of the "Avsandare" (sender) column in NORDEA CSV
       */
      inline std::optional<std::size_t> find_avsandare_column_index(CSV::Table::Heading const& heading) {
        std::initializer_list<std::string_view> avsandare_keywords = {
          "avsandare", "avsändare"
        };

        for (std::size_t i = 0; i < heading.size(); ++i) {
          if (contains_any_keyword(heading[i], avsandare_keywords)) {
            return i;
          }
        }
        return std::nullopt;
      }

      /**
       * Extract NORDEA account number from the "Avsandare" column
       *
       * The account number appears in the sender column (e.g., "51 86 87-9")
       * We look for the first non-empty value in that column across all data rows.
       */
      inline std::string extract_nordea_account(CSV::Table const& table) {
        auto maybe_col_idx = find_avsandare_column_index(table.heading);
        if (!maybe_col_idx) {
          return "";
        }

        std::size_t col_idx = *maybe_col_idx;

        // Skip the first row if it's the header (rows[0] == heading in current implementation)
        // Look for the first non-empty value in the Avsandare column
        for (auto const& row : table.rows) {
          if (col_idx < row.size()) {
            std::string const& cell_value = row[col_idx];
            // Skip empty cells and the header cell
            if (!cell_value.empty() && !contains_any_keyword(cell_value, {"avsandare", "avsändare"})) {
              return cell_value;
            }
          }
        }

        return "";
      }

      /**
       * Check if the CSV::Table content indicates an SKV (tax agency) format
       *
       * SKV CSV files have specific patterns:
       * - Headers or content containing: "Ingaende saldo", "Utgaende saldo", "Obetalt"
       * - Organisation number format: 10 digits like "5567828172" or "556782-8172"
       * - The newer format (sz_SKV_csv_20251120) has company name and org number in first row
       */
      inline bool is_skv_csv(CSV::Table const& table) {
        std::initializer_list<std::string_view> skv_keywords = {
          "ingaende saldo", "ingående saldo", "ing saldo",
          "utgaende saldo", "utgående saldo", "utg saldo",
          "obetalt",
          "intaktsranta", "intäktsränta",
          "moms"
        };

        // Check heading first
        for (auto const& col : table.heading) {
          if (contains_any_keyword(col, skv_keywords)) {
            return true;
          }
        }

        // Check content rows
        for (auto const& row : table.rows) {
          for (auto const& cell : row) {
            if (contains_any_keyword(cell, skv_keywords)) {
              return true;
            }
          }
        }

        return false;
      }

      /**
       * Search for a 10-digit organisation number in SKV data
       *
       * Organisation numbers can appear as:
       * - Pure 10 digits: "5567828172"
       * - With dash: "556782-8172"
       * - In text like "Inbetalning fran organisationsnummer XXXXXXXXXX"
       * - In SK reference: "SK5567828172"
       */
      inline std::optional<std::string> find_skv_org_number(CSV::Table const& table) {
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
      }

    } // namespace account_id_detection

    /**
     * Extract AccountID from a CSV::Table and pair it with the table
     *
     * Detection strategy:
     * 1. Check if this is a NORDEA CSV (by header keywords)
     *    - If yes: return MDTable<AccountID>{{"NORDEA", account_number}, table}
     *
     * 2. Check if this is an SKV CSV (by content patterns)
     *    - If yes with org number found: return MDTable<AccountID>{{"SKV", org_number}, table}
     *    - If yes without org number: return MDTable<AccountID>{{"SKV", ""}, table}
     *
     * 3. Unknown/Generic CSV (fully unknown - no prefix, no value):
     *    - Return std::nullopt (failure case)
     *
     * 4. Invalid/empty table:
     *    - Return std::nullopt
     */
    inline std::optional<CSV::MDTable<AccountID>> to_account_id_ed(CSV::Table const& table) {
      using namespace account_id_detection;

      logger::development_trace("to_account_id_ed: Starting AccountID extraction");

      // Check for fundamentally invalid table
      if (table.heading.size() == 0 && table.rows.empty()) {
        logger::development_trace("to_account_id_ed: Empty table, returning nullopt");
        return std::nullopt;
      }

      // Step 1: Check for NORDEA format
      if (is_nordea_csv(table.heading)) {
        std::string account_number = extract_nordea_account(table);
        logger::development_trace("to_account_id_ed: Detected NORDEA CSV, account: '{}'", account_number);
        return CSV::MDTable<AccountID>{AccountID{"NORDEA", account_number}, table};
      }

      // Step 2: Check for SKV format
      if (is_skv_csv(table)) {
        auto maybe_org_number = find_skv_org_number(table);
        std::string org_number = maybe_org_number.value_or("");
        logger::development_trace("to_account_id_ed: Detected SKV CSV, org number: '{}'", org_number);
        return CSV::MDTable<AccountID>{AccountID{"SKV", org_number}, table};
      }

      // Step 3: Unknown format - fully unknown AccountID, return nullopt (failure)
      logger::development_trace("to_account_id_ed: Unknown CSV format, returning nullopt");
      return std::nullopt;
    }

  } // namespace project
} // namespace CSV
