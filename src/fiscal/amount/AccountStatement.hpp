#pragma once

#include "TaggedAmountFramework.hpp"
#include "csv/projections.hpp"
#include <expected>
#include <variant>

// Delta represents a T value change
template <typename T>
struct Delta {
  T m_v;
};

// State represents a T value
template <typename T>
struct State {
  T m_v;
};

using AccountStatementEntry = std::variant<Delta<TaggedAmount>,State<TaggedAmount>>;
using AccountStatementEntries = std::vector<AccountStatementEntry>;

// Models transactions of an account statment (e.g., from a CSV account statement file)
class AccountStatement {
public:
  struct Meta {
    std::optional<std::string> m_maybe_account_irl_id;
  };
  AccountStatement(AccountStatementEntries const& entries,Meta meta = {});
  
  AccountStatementEntries const& entries() const { return m_entries; }
  Meta const& meta() const { return m_meta; }
  
private:
  Meta m_meta;
  AccountStatementEntries m_entries;
}; // AccountStatement

using ExpectedAccountStatement = std::expected<AccountStatement,std::string>;

namespace CSV {
  namespace project {
    ExpectedAccountStatement to_account_statement(CSV::project::HeadingId const& csv_heading_id, CSV::OptionalTable const& maybe_csv_table);
  }
}

