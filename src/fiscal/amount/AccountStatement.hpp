#pragma once

#include "TaggedAmountFramework.hpp"
#include "csv/csv.hpp"
#include <expected>

// Models transactions of an account statment (e.g., from a CSV account statement file)
class AccountStatement {
public:

private:
  TaggedAmounts m_tas;
}; // AccountStatement

using ExpectedAccountStatement = std::expected<AccountStatement,std::string>;

namespace CSV {
  namespace project {
    ExpectedAccountStatement to_account_statement(CSV::OptionalTable const& maybe_csv_table);
  }
}

