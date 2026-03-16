#include "csv_to_statement_id_ed.hpp"
#include "statement_table_meta.hpp"

namespace account {
  namespace statement {
    namespace maybe {

      std::optional<CSV::MDTable<maybe::table::TableMeta>> to_statement_id_ed_step(CSV::Table const& table) {
        logger::scope_logger log_raii(logger::development_trace,"to_statement_id_ed_step",logger::LogToConsole::OFF);

        auto table_meta = table::generic_like_to_statement_table_meta(table);

        if (table_meta.column_mapping.is_valid()) return CSV::MDTable<maybe::table::TableMeta>{table_meta,table};

        logger::development_trace("to_statement_id_ed_step: Unknown Account Statement format, returning nullopt");

        return std::nullopt;

      }


    } // maybe

    namespace monadic {
      AnnotatedMaybe<CSV::MDTable<maybe::table::TableMeta>> to_statement_id_ed_step(CSV::Table const& table) {

        auto to_msg = [](CSV::MDTable<maybe::table::TableMeta> const& result) -> std::string {
          return std::format(
            "transaction candidates:{}[{}..end:{}], columns[date:{} amount:{} saldo:{} description:{}]"
            ,result.meta.statement_mapping.transaction_candidates_count()
            ,result.meta.statement_mapping.tix_begin
            ,result.meta.statement_mapping.tix_end
            ,result.meta.column_mapping.date_column
            ,result.meta.column_mapping.transaction_amount_column
            ,result.meta.column_mapping.saldo_amount_column
            ,result.meta.column_mapping.description_column);
        };

        auto f =  cratchit::functional::to_annotated_maybe_f(
           account::statement::maybe::to_statement_id_ed_step
          ,"statement id:ed table"
          ,to_msg);

        return f(table);

      }

    } // monadic

  } // statement
} // account