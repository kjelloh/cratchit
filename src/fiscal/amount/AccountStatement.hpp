#pragma once

#include "TaggedAmountFramework.hpp"
#include "csv/projections.hpp"
#include <expected>
#include <variant>

// 20250803 - Experimental / KoH
//            Notion: Delta = State - State
//            So far it seems this aproach is not worth the effort?
//            Seems to require cumbersome boilerplate to process
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
  
  const AccountStatementEntries& entries() const { return m_entries; }
  const OptionalAccountDescriptor& account_descriptor() const { return m_account_descriptor; }
  
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

