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

        // ColumnMapping detect_columns_from_header(CSV::Table::Heading const& heading);
        // ColumnMapping detect_columns_from_data(CSV::Table::Rows const& rows);
        ColumnMapping to_column_mapping(CSV::Table const& table);

        bool is_ignorable_row(CSV::Table::Row const& row, ColumnMapping const& mapping);

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

      OptionalAccountStatementEntries csv_table_to_account_statement_entries(CSV::Table const& table);
      std::optional<AccountStatement> csv_table_to_account_statement_step(CSV::Table const& table,AccountID const& account_id);

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
