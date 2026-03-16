#pragma once

#include "fiscal/amount/AmountFramework.hpp" // Amount,...
#include "csv/statement_table_meta.hpp" // AccountID,
#include <expected>
#include <variant>
#include <map>

/**
 * Account Statement Entry - represents a single transaction in an account statement
 *
 * An account statement entry consists of:
 * - Transaction date (when the transaction occurred)
 * - Transaction amount (the monetary value, can be positive or negative)
 * - Transaction caption (description of the transaction)
 */
struct AccountStatementEntry {
  Date transaction_date;
  Amount transaction_amount;
  std::string transaction_caption;
  using Tags = std::map<std::string, std::string>;
  Tags transaction_tags;

  AccountStatementEntry(Date date, Amount amount, std::string caption,Tags tags = {})
    : transaction_date(date)
    , transaction_amount(amount)
    , transaction_caption(std::move(caption))
    , transaction_tags{tags}
  {}
};

using AccountStatementEntries = std::vector<AccountStatementEntry>;
using OptionalAccountStatementEntries = std::optional<AccountStatementEntries>;
using AccountID = account::statement::AccountID;

inline AccountID make_account_id(std::string const& prefix, std::string const& value) {
  return AccountID{.m_prefix = prefix, .m_value = value};
}

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
    std::optional<AccountID> m_maybe_account_irl_id;
  };
  AccountStatement(AccountStatementEntries const& entries,Meta meta = {});
  
  AccountStatementEntries const& entries() const { return m_entries; }
  Meta const& meta() const { return m_meta; }
  
private:
  Meta m_meta;
  AccountStatementEntries m_entries;
}; // AccountStatement

using ExpectedAccountStatement = std::expected<AccountStatement,std::string>;

// Now in projections unit
// namespace CSV {
//   namespace project {
//     ExpectedAccountStatement to_account_statement(CSV::project::deprecated::HeadingId const& csv_heading_id, CSV::OptionalTable const& maybe_csv_table);
//   }
// }

