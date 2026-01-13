#pragma once

#include "csv/csv.hpp" // CSV::Table, CSV::MDTable
#include "fiscal/amount/AmountFramework.hpp"
#include "FiscalPeriod.hpp"
#include "fiscal/amount/AccountStatement.hpp"
#include "logger/log.hpp" // logger::...
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <algorithm>
#include <ranges>

namespace account {

  namespace statement {

    namespace maybe {

      namespace table {


        enum class FieldType {
           Unknown
          ,Empty
          ,Date
          ,Amount
          ,Text 
          ,Undefined
        };

        struct RowMap {
          std::map<FieldType,std::vector<unsigned>> ixs;
        };

        RowMap to_row_map(CSV::Table::Row const& row);

        using RowsMap = std::vector<RowMap>;

        RowsMap to_rows_map(CSV::Table::Rows const& rows);

        /**
        * Column Detection Result
        *
        * Identifies which columns in a CSV::Table correspond to required account statement fields.
        * Uses -1 to indicate "not found".
        */
        struct ColumnMapping {
          int date_column = -1;
          int transaction_amount_column = -1;  // Column with transaction delta/change
          int saldo_amount_column = -1;        // Column with running balance (optional)
          int description_column = -1;         // Primary description column
          std::vector<int> additional_description_columns;  // Additional columns to concatenate

          bool is_valid() const {
            return date_column >= 0 && transaction_amount_column >= 0 && description_column >= 0;
          }
        }; // ColumnMapping

        /**
        * Detect column mapping from CSV table header
        *
        * Strategy: Look for keywords in header column names to identify columns
        * - Date: "date", "datum", "bokföringsdag", "dag"
        * - Amount: "amount", "belopp", "suma"
        * - Description: "description", "namn", "rubrik", "meddelande", "text"
        */
        inline ColumnMapping detect_columns_from_header(CSV::Table::Heading const& heading) {

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

        ColumnMapping detect_columns_from_data(CSV::Table::Rows const& rows);

        /**
        * Determine if a CSV row is "ignorable" (e.g., balance rows in SKV format)
        *
        * Ignorable rows:
        * - Rows where first column is empty (balance rows)
        * - Rows that contain "saldo" in the description
        * - Rows that contain "ingående" or "utgående"
        */
        inline bool is_ignorable_row(CSV::Table::Row const& row, ColumnMapping const& mapping) {
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

          return false;
        } // is_ignorable_row

        /**
        * Extract account statement entry from a CSV row
        *
        * Returns nullopt if:
        * - Row is ignorable
        * - Required fields are missing
        * - Date cannot be parsed
        * - Amount cannot be parsed
        */
        inline std::optional<AccountStatementEntry> extract_entry_from_row(
            CSV::Table::Row const& row,
            ColumnMapping const& mapping) {

          // Check if row is ignorable
          if (is_ignorable_row(row, mapping)) {
            return std::nullopt;
          }

          // Validate column mapping
          if (!mapping.is_valid()) {
            return std::nullopt;
          }

          // Extract and validate date
          if (static_cast<size_t>(mapping.date_column) >= row.size() ||
              row[mapping.date_column].size() == 0) {
            return std::nullopt;
          }

          auto maybe_date = to_date(row[mapping.date_column]);
          if (!maybe_date) {
            return std::nullopt;
          }

          // Extract and validate transaction amount
          if (static_cast<size_t>(mapping.transaction_amount_column) >= row.size() ||
              row[mapping.transaction_amount_column].size() == 0) {
            return std::nullopt;
          }

          auto maybe_amount = to_amount(row[mapping.transaction_amount_column]);
          if (!maybe_amount) {
            return std::nullopt;
          }

          // Extract description (concatenate multiple columns if needed)
          std::string description;

          if (static_cast<size_t>(mapping.description_column) < row.size()) {
            description = row[mapping.description_column];
          }

          // Add additional description columns
          for (int col : mapping.additional_description_columns) {
            if (col != mapping.description_column &&
                static_cast<size_t>(col) < row.size() &&
                row[col].size() > 0) {
              if (description.size() > 0) {
                description += " | ";
              }
              description += row[col];
            }
          }

          // Description cannot be empty
          if (description.size() == 0) {
            return std::nullopt;
          }

          return AccountStatementEntry(*maybe_date, *maybe_amount, description);
        } // extract_entry_from_row

      } // table

      inline OptionalAccountStatementEntries csv_table_to_account_statement_entries(CSV::Table const& table) {
        logger::scope_logger log_raii{logger::development_trace, "csv_table_to_account_statement_entries(table)"};
        
        // Try to detect columns from header first
        table::ColumnMapping mapping = table::detect_columns_from_header(table.heading);

        // If header detection failed, try data-based detection
        if (!mapping.is_valid()) {
          mapping = table::detect_columns_from_data(table.rows);
        }

        // If we still can't detect required columns, fail
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

      inline std::optional<AccountStatement> csv_table_to_account_statement_step(
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

      inline std::optional<AccountStatement> account_id_ed_to_account_statement_step(
          CSV::MDTable<AccountID> const& account_id_ed) {
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

  } // statement

} // account
