#pragma once

#include "core/amount.hpp"
#include "core/ar/statement.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cratchit::ar {

// Flerårsöversikt: key figures per year (index 0 first), from the statement
struct Overview {
  std::vector<Amount> net_sales;                      // Nettoomsättning, öre as shown
  std::vector<Amount> result_after_financial_items;   // Resultat efter finansiella poster, öre as shown
  // Soliditet in tenths of a percent, e.g. 962 for 96,2 %; nothing when Summa tillgångar is 0
  std::vector<std::optional<std::int64_t>> solidity;
  // What soliditet is computed from, öre as shown
  std::vector<Amount> equity;            // Summa eget kapital
  std::vector<Amount> untaxed_reserves;  // Summa obeskattade reserver
  std::vector<Amount> assets;            // Summa tillgångar
};

// The share of untaxed reserves that counts as equity: 1 - 20,6 % tax, in thousandths
constexpr std::int64_t equity_share_of_untaxed_reserves = 794;

// Soliditet = (Summa eget kapital + 79,4 % of Summa obeskattade reserver) / Summa
// tillgångar, the K2 definition (justerat eget kapital in procent av
// balansomslutningen), rounded to a tenth of a percent half away from zero.
// Lines missing from the statement count as 0.
Overview build_overview(Statement const& statement);

// Öre in whole thousands of kronor (tkr), half away from zero
std::int64_t to_tkr(Amount amount);

// Tenths of a percent with a decimal comma, e.g. "96,2"
std::string format_percent(std::int64_t tenths);

} // namespace cratchit::ar
