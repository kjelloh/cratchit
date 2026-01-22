#pragma once

#include "fiscal/amount/AccountStatement.hpp"
#include "fiscal/amount/TaggedAmountFramework.hpp"
#include "functional/maybe.hpp"
#include "fiscal/amount/AmountFramework.hpp"
#include "domain/csv_to_account_statement.hpp"
#include "logger/log.hpp"
#include <optional>
#include <string>
#include <ranges>
#include <algorithm>

namespace tas {

  namespace maybe {

    std::optional<TaggedAmounts> account_statement_to_tagged_amounts_step(AccountStatement const& statement);

  } // maybe

  namespace monadic {
    AnnotatedMaybe<TaggedAmounts> account_statement_to_tagged_amounts_step(AccountStatement const& statement);
  }

  // Monadic Maybe: step #? .. #? shortcut
  inline std::optional<TaggedAmounts> csv_table_to_tagged_amounts_shortcut(
      CSV::Table const& table,
      AccountID const& account_id) {
    logger::scope_logger log_raii{logger::development_trace,
      "tas::csv_table_to_tagged_amounts_shortcut(table, account_id)"};

    // Step 7: CSV::Table + AccountID -> AccountStatement
    auto maybe_statement = account::statement::maybe::csv_table_to_account_statement_step(table, account_id);

    if (!maybe_statement) {
      logger::development_trace("Step 7 failed: csv_table_to_account_statement_step returned nullopt");
      return std::nullopt;
    }

    // Step 8: AccountStatement -> TaggedAmounts
    return maybe::account_statement_to_tagged_amounts_step(*maybe_statement);
  }

} // tas
