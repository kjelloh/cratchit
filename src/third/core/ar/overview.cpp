#include "core/ar/overview.hpp"

#include <format>

namespace cratchit::ar {

namespace {

// a / b rounded half away from zero; b not 0
std::int64_t rounded_quotient(std::int64_t a, std::int64_t b) {
  bool const negative = (a < 0) != (b < 0);
  std::int64_t const abs_a = a < 0 ? -a : a;
  std::int64_t const abs_b = b < 0 ? -b : b;
  std::int64_t const quotient = (abs_a + abs_b / 2) / abs_b;
  return negative ? -quotient : quotient;
}

// The exact amounts of the first line with text outside the notes, else zeros
std::vector<Amount> exact_of(Statement const& statement, std::string_view text, std::size_t years) {
  for (auto const& line : statement.lines) {
    if (!line.in_notes && line.kind != LineKind::heading && line.text == text) {
      return line.exact;
    }
  }
  return std::vector<Amount>(years, 0);
}

} // namespace

std::int64_t to_tkr(Amount amount) {
  return rounded_quotient(amount, 100000);
}

std::string format_percent(std::int64_t tenths) {
  auto const magnitude = tenths < 0 ? -tenths : tenths;
  return std::format("{}{},{}", tenths < 0 ? "-" : "", magnitude / 10, magnitude % 10);
}

Overview build_overview(Statement const& statement) {
  std::size_t const years = statement.lines.empty() ? 0 : statement.lines.front().exact.size();
  Overview overview;
  overview.net_sales = exact_of(statement, "Nettoomsättning", years);
  overview.result_after_financial_items = exact_of(statement, "Resultat efter finansiella poster", years);
  overview.equity = exact_of(statement, "Summa eget kapital", years);
  overview.untaxed_reserves = exact_of(statement, "Summa obeskattade reserver", years);
  overview.assets = exact_of(statement, "Summa tillgångar", years);
  for (std::size_t year = 0; year < years; ++year) {
    if (overview.assets[year] == 0) {
      overview.solidity.push_back(std::nullopt);
      continue;
    }
    // (equity + 0,794 * reserves) / assets * 1000 tenths of a percent
    auto const adjusted = overview.equity[year] * 1000 + overview.untaxed_reserves[year] * equity_share_of_untaxed_reserves;
    overview.solidity.push_back(rounded_quotient(adjusted, overview.assets[year]));
  }
  return overview;
}

} // namespace cratchit::ar
