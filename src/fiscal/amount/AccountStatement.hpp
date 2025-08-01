#pragma once

#include "TaggedAmountFramework.hpp"
#include "csv/projections.hpp"
#include <expected>
#include <variant>

template <typename T>
struct Delta {
  T m_t;
};

template <typename T>
struct State {
  T m_t;
};

using AccountStatementEntry = std::variant<Delta<TaggedAmount>,State<TaggedAmount>>;
using AccountStatementEntries = std::vector<AccountStatementEntry>;

// Models transactions of an account statment (e.g., from a CSV account statement file)
class AccountStatement {
public:
  using AccountDescriptor = std::string;
  using OptionalAccountDescriptor = std::optional<AccountDescriptor>;
  AccountStatement(AccountStatementEntries const& entries,OptionalAccountDescriptor account_descriptor = std::nullopt);
private:
  OptionalAccountDescriptor m_account_descriptor;
  AccountStatementEntries m_entries;
}; // AccountStatement

using ExpectedAccountStatement = std::expected<AccountStatement,std::string>;

namespace CSV {
  namespace project {
    ExpectedAccountStatement to_account_statement(CSV::project::HeadingId const& csv_heading_id, CSV::OptionalTable const& maybe_csv_table);
  }
}

