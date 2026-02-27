#include "csv_to_account_statement.hpp"
#include "fiscal/amount/TaggedAmountFramework.hpp"
#include <set>

namespace account {

  namespace statement {

    namespace maybe {

      namespace table {

        std::optional<AccountStatementEntry> extract_entry_from_row(
          CSV::Table::Row const& row,
          ColumnMapping const& mapping) {

          // Validate column mapping
          if (!mapping.is_valid()) {
            return std::nullopt;
          }

          if (    static_cast<size_t>(mapping.date_column) >= row.size() 
               || row[mapping.date_column].size() == 0) {
            return std::nullopt;
          }

          auto maybe_date = to_date(row[mapping.date_column]);
          if (!maybe_date) {
            return std::nullopt;
          }

          if (    static_cast<size_t>(mapping.transaction_amount_column) >= row.size() 
               || row[mapping.transaction_amount_column].size() == 0) {
            return std::nullopt;
          }

          auto maybe_amount = to_amount(row[mapping.transaction_amount_column]);
          if (!maybe_amount) {
            return std::nullopt;
          }

          std::string description;

          if (static_cast<size_t>(mapping.description_column) < row.size()) {
            description = row[mapping.description_column];
          }

          for (int col : mapping.additional_description_columns) {
            if (     (col != mapping.description_column)
                 &&  (static_cast<size_t>(col) < row.size()) 
                 &&  (row[col].size() > 0)) {

              if (description.size() > 0) description += " | ";
              description += row[col];

            }
          }

          if (description.size() == 0) {
            return std::nullopt;
          }

          return AccountStatementEntry(*maybe_date, *maybe_amount, description);

        } // extract_entry_from_row

      } // table

      std::optional<AccountStatement> statement_id_ed_to_account_statement_step(CSV::MDTable<table::TableMeta> const& statement_id_ed) {
        logger::scope_logger log_raii{
           logger::development_trace
          ,"statement_id_ed_to_account_statement_step(statement_id_ed)"};

        auto const& [table_meta,table] = statement_id_ed;
        auto mapping = table_meta.column_mapping;

        if (!mapping.is_valid()) return std::nullopt;

        AccountStatementEntries candidate;
        for (auto const& row : table.rows) {
          if (auto maybe_entry = table::extract_entry_from_row(row, mapping)) {
            candidate.push_back(*maybe_entry);
          }
        }

        auto const& account_id = table_meta.account_id;
        AccountStatement::Meta meta{.m_maybe_account_irl_id = account_id};

        logger::development_trace("Creating AccountStatement with {} entries and account_id: {}",
          candidate.size(), account_id.to_string());

        return AccountStatement(candidate, meta);
      } // statement_id_ed_to_account_statement_step

    } // maybe

    namespace monadic {

      AnnotatedMaybe<AccountStatement> statement_id_ed_to_account_statement_step(CSV::MDTable<maybe::table::TableMeta> const& statement_id_ed) {

        auto to_msg = [](AccountStatement const& result) -> std::string {
          return std::format(
            "{} : {} entries"
            ,result.meta().m_maybe_account_irl_id.value_or(AccountID{"??,??"}).to_string()
            ,result.entries().size()
            );
        };

        auto f = cratchit::functional::_to_annotated_maybe_f(
           account::statement::maybe::statement_id_ed_to_account_statement_step
          ,"account statement"
          ,to_msg);

        return f(statement_id_ed);

      }


    } // monadic
  } // statement
} // account