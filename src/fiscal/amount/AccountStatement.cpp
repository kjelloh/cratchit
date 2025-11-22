#include "AccountStatement.hpp"
#include "../../logger/log.hpp"
#include <sstream>

AccountStatement:: AccountStatement(
   AccountStatementEntries const& entries
  ,Meta meta)
    :  m_entries{entries}
      ,m_meta{meta} {}

// Now In csv/projections unit
// namespace CSV {
//   namespace project {
//     ExpectedAccountStatement to_account_statement(CSV::project::HeadingId const& csv_heading_id, CSV::OptionalTable const& maybe_csv_table) {
