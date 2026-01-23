#include "csv_to_account_id.hpp"

namespace account {
  namespace statement {
    namespace maybe {

      std::optional<CSV::MDTable<AccountID>> to_account_id_ed_step(CSV::Table const& table) {

        logger::scope_logger log_raii(logger::development_trace,"to_account_id_ed_step");

        logger::development_trace("to_account_id_ed_step: Starting AccountID extraction");

        // Check for fundamentally invalid table
        if (table.heading.size() == 0 && table.rows.empty()) {
          logger::development_trace("to_account_id_ed_step: Empty table, returning nullopt");
          return std::nullopt;
        }

        if (account::statement::NORDEA::is_account_statement_table(table)) {
          std::string account_number = account::statement::NORDEA::to_account_no(table).value_or("");
          logger::development_trace("to_account_id_ed_step: Detected NORDEA account: '{}'", account_number);
          return CSV::MDTable<AccountID>{AccountID{"NORDEA", account_number}, table};
        }

        if (account::statement::SKV::is_account_statement_table(table)) {
          auto maybe_org_number = account::statement::SKV::to_account_no(table);
          std::string org_number = maybe_org_number.value_or("");
          logger::development_trace("to_account_id_ed_step: Detected SKV account for org: '{}'", org_number);
          return CSV::MDTable<AccountID>{AccountID{"SKV", org_number}, table};
        }

        // Unknown format - fully unknown AccountID, return nullopt (failure)
        logger::development_trace(
          "to_account_id_ed_step: Unknown Account Statement format, returning nullopt");
        return std::nullopt;
      }


    } // maybe

    namespace monadic {
      AnnotatedMaybe<CSV::MDTable<AccountID>> to_account_id_ed_step(CSV::Table const& table) {
        using cratchit::functional::to_annotated_maybe_f;

        auto f =  to_annotated_maybe_f(
           account::statement::maybe::to_account_id_ed_step
          ,"Failed to identify account statement csv table account id");

        return f(table);

      }

    } // monadic

  } // statement
} // account