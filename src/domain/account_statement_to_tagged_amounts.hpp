#pragma once

#include "fiscal/amount/AccountStatement.hpp"
#include "fiscal/amount/TaggedAmount.hpp"
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

} // tas
