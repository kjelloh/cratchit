#pragma once

#include "fiscal/amount/AccountStatement.hpp"
#include "fiscal/amount/TaggedAmountFramework.hpp"
#include "fiscal/amount/AmountFramework.hpp"
#include "domain/csv_to_account_statement.hpp"
#include "logger/log.hpp"
#include <optional>
#include <string>
#include <ranges>
#include <algorithm>

namespace tas {

  namespace maybe {

    // Monadic Maybe: accouht statement -> TaggedAmounts
    inline std::optional<TaggedAmounts> account_statement_to_tagged_amounts_step(
        AccountStatement const& statement) {
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
    }

  } // maybe

  // Monadic Maybe: step #? .. #? shortcut
  inline std::optional<TaggedAmounts> csv_table_to_tagged_amounts_shortcut(
      CSV::Table const& table,
      AccountID const& account_id) {
    logger::scope_logger log_raii{logger::development_trace,
      "tas::csv_table_to_tagged_amounts_shortcut(table, account_id)"};

    // Step 7: CSV::Table + AccountID -> AccountStatement
    auto maybe_statement = account::statement::csv_table_to_account_statement_step(table, account_id);

    if (!maybe_statement) {
      logger::development_trace("Step 7 failed: csv_table_to_account_statement_step returned nullopt");
      return std::nullopt;
    }

    // Step 8: AccountStatement -> TaggedAmounts
    return maybe::account_statement_to_tagged_amounts_step(*maybe_statement);
  }

} // tas
