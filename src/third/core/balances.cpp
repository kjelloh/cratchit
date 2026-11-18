#include "core/balances.hpp"

namespace cratchit {

Kronor to_kronor(Amount amount) {
  return amount >= 0 ? (amount + 50) / 100 : -((-amount + 50) / 100);
}

Balances year_balances(sie::Document const& document, int year) {
  Balances balances;
  for (auto const& balance : document.closing) {
    if (balance.year == year) {
      balances[balance.account] += balance.amount;
    }
  }
  for (auto const& result : document.results) {
    if (result.year == year) {
      balances[result.account] += result.amount;
    }
  }
  return balances;
}

} // namespace cratchit
