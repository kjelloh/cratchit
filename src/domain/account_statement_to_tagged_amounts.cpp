#include "account_statement_to_tagged_amounts.hpp"

namespace tas {

  namespace maybe {

    std::optional<TaggedAmounts> account_statement_to_tagged_amounts_step(AccountStatement const& statement) {
      logger::scope_logger log_raii{logger::development_trace,
        "account_statement_to_tagged_amounts_step(statement)"};

      // Extract account ID string from meta
      // If no AccountID is present, use empty string
      std::string account_id_string;
      if (statement.meta().m_maybe_account_irl_id.has_value()) {
        account_id_string = statement.meta().m_maybe_account_irl_id->to_string();
      }

      logger::development_trace("Processing statement with {} entries, account_id: '{}'",
        statement.entries().size(), account_id_string);

      // Transform each AccountStatementEntry to TaggedAmount
      TaggedAmounts result;
      result.reserve(statement.entries().size());

      for (auto const& entry : statement.entries()) {
        // Convert Amount to CentsAmount using existing conversion function
        CentsAmount cents_amount = to_cents_amount(entry.transaction_amount);

        // Build tags map with required "Text" and "Account" entries
        TaggedAmount::Tags tags = {
          {"Text", entry.transaction_caption},
          {"Account", account_id_string}
        };

        // Merge any existing transaction_tags from the entry
        // (preserving the "Text" and "Account" values we just set)
        for (auto const& [key, value] : entry.transaction_tags) {
          if (key != "Text" && key != "Account") {
            tags[key] = value;
          }
        }

        // Create TaggedAmount with date, cents_amount, and tags
        result.emplace_back(entry.transaction_date, cents_amount, std::move(tags));

        logger::development_trace("Created TaggedAmount: cents={}, text='{}'",
          to_amount_in_cents_integer(cents_amount), entry.transaction_caption);
      }

      logger::development_trace("Returning {} TaggedAmounts", result.size());

      return result;
    } // account_statement_to_tagged_amounts_step

  }

  namespace monadic {

    AnnotatedMaybe<TaggedAmounts> account_statement_to_tagged_amounts_step(AccountStatement const& statement) {

      auto to_msg = [](TaggedAmounts const& result) -> std::string {
        return std::format(
          "{} amounts"
          ,result.size());
      };
      auto f = cratchit::functional::to_annotated_maybe_f(
           tas::maybe::account_statement_to_tagged_amounts_step
          ,"tagged ampunts"
          ,to_msg);

      return f(statement);

    }

  } // monadic
} // tas
