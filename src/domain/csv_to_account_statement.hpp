#pragma once

#include "csv/csv.hpp" // CSV::Table, CSV::MDTable
#include "fiscal/amount/AmountFramework.hpp"
#include "FiscalPeriod.hpp"
#include "fiscal/amount/AccountStatement.hpp"
#include "functional/maybe.hpp"
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

        RowMap to_row_map(CSV::Table::Row const& row);

        using RowsMap = std::vector<RowMap>;

        RowsMap to_rows_map(CSV::Table::Rows const& rows);

        bool is_ignorable_row(CSV::Table::Row const& row, ColumnMapping const& mapping);

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

        // Expose generic for testing
        std::optional<StatementMapping> generic_like_to_statement_mapping(CSV::Table const& table);
        std::optional<ColumnMapping> generic_like_to_column_mapping(CSV::MDTable<StatementMapping> const& mapped_table);
        std::optional<AccountID> generic_like_to_account_id(CSV::MDTable<StatementMapping> const& mapped_table);
        TableMeta generic_like_to_statement_table_meta(CSV::Table const& table);

        // Expose for testing 
        ColumnMapping nordea_like_to_column_mapping(CSV::Table const& table);
        ColumnMapping to_column_mapping(CSV::Table const& table);

      } // table


      // maybe steps
      OptionalAccountStatementEntries csv_table_to_account_statement_entries(CSV::Table const& table);
      std::optional<AccountStatement> csv_table_to_account_statement_step(CSV::Table const& table,AccountID const& account_id);
      std::optional<AccountStatement> statement_id_ed_to_account_statement_step(CSV::MDTable<account::statement::TableMeta> const& statement_id_ed);

    } // maybe

    namespace monadic {

      AnnotatedMaybe<AccountStatement> statement_id_ed_to_account_statement_step(CSV::MDTable<account::statement::TableMeta> const& statement_id_ed);

    }

  } // statement

} // account
