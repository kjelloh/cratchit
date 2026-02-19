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

        bool is_ignorable_row(CSV::Table::Row const& row, ColumnMapping const& mapping);

        std::optional<AccountStatementEntry> extract_entry_from_row(
            CSV::Table::Row const& row,
            ColumnMapping const& mapping);

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
