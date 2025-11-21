#pragma once

#include "domain/csv_to_account_statement.hpp"
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

struct DomainPrefixedId {
  std::string m_prefix; // E.g., NORDEA,PG,BG,IBAN,SKV,
  std::string m_value; // E.g. a bank account, a PG account etc...
  std::string to_string() const {
    return std::format(
       "{}{}"
      ,((m_prefix.size()>0)?m_prefix:std::string{})
      ,m_value);
  }
};

// Models transactions of an account statment (e.g., from a CSV account statement file)
class AccountStatement {
public:
  // 'About' this account statement
  // Note: Consider to pair AccountStatement with other meta-data that is less
  //       tightly (more transient) to its nature. Like a source file path
  //       or such that is not relevant for core book-keeping operations on 
  //       account statement content.
  // TODO: Make m_maybe_account_irl_id required when mapping to accouning
  //       is in place. The optional nature allows for manual mapping 
  //       to e.g., BAS account by the user.
  struct Meta {
    std::optional<DomainPrefixedId> m_maybe_account_irl_id;
  };
  AccountStatement(domain::AccountStatementEntries const& entries,Meta meta = {});
  
  domain::AccountStatementEntries const& entries() const { return m_entries; }
  Meta const& meta() const { return m_meta; }
  
private:
  Meta m_meta;
  domain::AccountStatementEntries m_entries;
}; // AccountStatement

using ExpectedAccountStatement = std::expected<AccountStatement,std::string>;

namespace CSV {
  namespace project {
    ExpectedAccountStatement to_account_statement(CSV::project::HeadingId const& csv_heading_id, CSV::OptionalTable const& maybe_csv_table);
  }
}

