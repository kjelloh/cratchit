#pragma once

#include "core/amount.hpp"
#include "core/ar/equity.hpp"
#include "core/ar/statement.hpp"
#include "core/sie/document.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit::ar {

// A voucher's transaction on a traced account
struct Posting {
  sie::Voucher const* voucher = nullptr;
  Amount amount = 0; // as booked
};

// An account of a traced line, all amounts as booked (debit positive)
struct TracedAccount {
  std::string account;
  std::string name;                 // from #KONTO, empty when not given
  std::vector<Amount> amounts;      // per year: #UB of a balance account, #RES of a result account
  std::optional<Amount> opening;    // #IB 0 of a balance account (class 1 and 2)
  std::vector<Posting> postings;    // year 0, by date
};

// A shown item of the resultaträkning or balansräkning and what it is made of
struct TracedLine {
  StatementLine const* line = nullptr;
  std::vector<TracedAccount> accounts; // by number
};

// Each shown item of the resultaträkning and balansräkning (not the notes) with the accounts the mapping puts on it that have an
// amount in any year, an opening balance or a posting in year 0. Year 0 goes
// down to the vouchers; the SIE file has none for the years before. The lines
// point into statement and document.
std::vector<TracedLine> trace_statement(Statement const& statement, sie::Document const& document);

// The #KONTO name of account, empty when the file does not give one
std::string account_name(sie::Document const& document, std::string_view account);

} // namespace cratchit::ar
