#pragma once

#include "core/amount.hpp"
#include "core/ar/layout.hpp"
#include "core/ar/statement.hpp"
#include "core/sie/document.hpp"

#include <string>
#include <vector>

namespace cratchit::ar {

// Förändringar i eget kapital of fiscal year 0, in öre as the årsredovisning
// shows equity: positive for capital, negative for a loss
struct EquityChanges {
  struct Row {
    std::string text;
    std::vector<Amount> amounts; // one per column, then the total
    // What the row is made of, as booked: for the first and last row the accounts' balances,
    // for a change the vouchers (in the document) with an equity transaction
    Balances accounts;
    std::vector<sie::Voucher const*> vouchers;
  };
  std::vector<std::string> columns; // the posts under Eget kapital with an amount
  std::vector<Row> rows;            // Belopp vid årets ingång, the changes with an amount, Belopp vid årets utgång
  std::vector<std::string> errors;  // a column whose opening and changes do not add up to its closing
  std::vector<std::string> notes;   // vouchers whose change is shown as Övriga förändringar
  // The accounts of each column, for tracing
  std::vector<AccountSet> column_accounts;
};

// The posts under the heading Eget kapital in the layout are the columns. A
// voucher's transactions on their accounts are a change, by its other accounts:
// - none: Balanseras i ny räkning (between the posts, e.g. 2099 to 2098)
// - 8999: Årets resultat
// - 2898 Outtagen vinstutdelning: Utdelning (a cancelled dividend nets out)
// - else, when all its equity transactions are on 2093: Erhållna aktieägartillskott
// - else Övriga förändringar, with a note
// Opening (#IB 0) plus the changes must be closing (#UB 0) in every column.
// The rows point into document's vouchers.
EquityChanges equity_changes(std::vector<Line> const& layout, sie::Document const& document);

} // namespace cratchit::ar
