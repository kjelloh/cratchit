#include "AccountStatement.hpp"

namespace CSV {
  namespace project {
    ExpectedAccountStatement to_account_statement(CSV::OptionalTable const& maybe_csv_table) {
      if (not maybe_csv_table) return std::unexpected("CSV -> Table failed");
      return std::unexpected("to_account_statement not yet implemented");
    }
  }
}
