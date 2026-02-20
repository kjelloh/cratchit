#include "csv_to_statement_id_ed.hpp"
#include "statement_table_meta.hpp"

namespace account {
  namespace statement {
    namespace maybe {

      std::optional<CSV::MDTable<maybe::table::TableMeta>> to_statement_id_ed_step(CSV::Table const& table) {

        logger::scope_logger log_raii(logger::development_trace,"to_statement_id_ed_step",logger::LogToConsole::ON);

        auto table_meta = table::generic_like_to_statement_table_meta(table);
        if (table_meta.column_mapping.is_valid()) return CSV::MDTable<maybe::table::TableMeta>{table_meta,table};

        // Unknown format - fully unknown AccountID, return nullopt (failure)
        logger::development_trace(
          "to_statement_id_ed_step: Unknown Account Statement format, returning nullopt");
        return std::nullopt;
      }


    } // maybe

    namespace monadic {
      AnnotatedMaybe<CSV::MDTable<maybe::table::TableMeta>> to_statement_id_ed_step(CSV::Table const& table) {
        using cratchit::functional::to_annotated_maybe_f;

        auto f =  to_annotated_maybe_f(
           account::statement::maybe::to_statement_id_ed_step
          ,"Failed to identify account statement csv table account id");

        return f(table);

      }

    } // monadic

  } // statement
} // account