#include "core/ar/equity.hpp"

#include "core/date.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <map>
#include <optional>

namespace cratchit::ar {

namespace {

// The changes in the order the årsredovisning lists them
constexpr std::array<std::string_view, 5> change_texts{
    "Erhållna aktieägartillskott", "Utdelning", "Balanseras i ny räkning", "Årets resultat", "Övriga förändringar",
};

// The item lines under the heading Eget kapital
std::vector<Line const*> equity_items(std::vector<Line> const& layout) {
  std::vector<Line const*> items;
  auto const heading = std::ranges::find_if(
      layout, [](Line const& line) { return line.kind == LineKind::heading && line.text == "Eget kapital"; });
  if (heading == layout.end()) {
    return items;
  }
  for (auto at = heading + 1; at != layout.end() && (at->kind == LineKind::item || at->level > heading->level); ++at) {
    if (at->kind == LineKind::item) {
      items.push_back(&*at);
    }
  }
  return items;
}

std::string_view change_of(std::vector<sie::Transaction const*> const& equity,
                           std::vector<sie::Transaction const*> const& other) {
  if (other.empty()) {
    return "Balanseras i ny räkning";
  }
  auto const has = [&other](std::string_view account) {
    return std::ranges::any_of(other, [account](auto const* transaction) { return transaction->account == account; });
  };
  if (has("8999")) {
    return "Årets resultat";
  }
  if (has("2898")) {
    return "Utdelning";
  }
  if (std::ranges::all_of(equity, [](auto const* transaction) { return transaction->account == "2093"; })) {
    return "Erhållna aktieägartillskott";
  }
  return "Övriga förändringar";
}

} // namespace

EquityChanges equity_changes(std::vector<Line> const& layout, sie::Document const& document) {
  EquityChanges changes;
  auto const items = equity_items(layout);
  auto const column_of = [&items](std::string_view account) -> std::optional<std::size_t> {
    for (std::size_t index = 0; index < items.size(); ++index) {
      if (items[index]->accounts.contains(account)) {
        return index;
      }
    }
    return std::nullopt;
  };
  // Shown as equity: credit positive
  std::vector<Amount> opening(items.size(), 0);
  std::vector<Amount> closing(items.size(), 0);
  Balances opening_accounts;
  Balances closing_accounts;
  for (auto const& balance : document.opening) {
    if (auto const column = column_of(balance.account); balance.year == 0 && column) {
      opening[*column] -= balance.amount;
      opening_accounts[balance.account] += balance.amount;
    }
  }
  for (auto const& balance : document.closing) {
    if (auto const column = column_of(balance.account); balance.year == 0 && column) {
      closing[*column] -= balance.amount;
      closing_accounts[balance.account] += balance.amount;
    }
  }
  std::map<std::string_view, std::vector<Amount>> by_change;
  std::map<std::string_view, std::vector<sie::Voucher const*>> vouchers_of;
  for (auto const& voucher : document.vouchers) {
    std::vector<sie::Transaction const*> equity;
    std::vector<sie::Transaction const*> other;
    for (auto const& transaction : voucher.transactions) {
      if (transaction.kind != sie::Transaction::Kind::removed) {
        (column_of(transaction.account) ? equity : other).push_back(&transaction);
      }
    }
    if (equity.empty()) {
      continue;
    }
    auto const change = change_of(equity, other);
    vouchers_of[change].push_back(&voucher);
    auto& amounts = by_change.try_emplace(change, items.size(), 0).first->second;
    for (auto const* transaction : equity) {
      amounts[*column_of(transaction->account)] -= transaction->amount;
    }
    if (change == "Övriga förändringar") {
      changes.notes.push_back(std::format("Voucher {} {} {} \"{}\" changes eget kapital; shown as Övriga förändringar",
                                          voucher.series, voucher.number, format_iso_date(voucher.date), voucher.text));
    }
  }
  // Columns and rows with an amount, and the total last
  std::vector<std::size_t> used;
  for (std::size_t column = 0; column < items.size(); ++column) {
    bool const any = opening[column] != 0 || closing[column] != 0 ||
                     std::ranges::any_of(by_change, [column](auto const& entry) { return entry.second[column] != 0; });
    if (any) {
      used.push_back(column);
      changes.columns.push_back(items[column]->text);
      changes.column_accounts.push_back(items[column]->accounts);
    }
  }
  auto const row = [&used](std::string_view text, std::vector<Amount> const& amounts) {
    EquityChanges::Row out{std::string(text), {}, {}, {}};
    Amount total = 0;
    for (auto const column : used) {
      out.amounts.push_back(amounts[column]);
      total += amounts[column];
    }
    out.amounts.push_back(total);
    return out;
  };
  changes.rows.push_back(row("Belopp vid årets ingång", opening));
  changes.rows.back().accounts = opening_accounts;
  for (auto const text : change_texts) {
    auto const found = by_change.find(text);
    if (found != by_change.end() && std::ranges::any_of(found->second, [](Amount amount) { return amount != 0; })) {
      changes.rows.push_back(row(text, found->second));
      changes.rows.back().vouchers = vouchers_of[text];
    }
  }
  changes.rows.push_back(row("Belopp vid årets utgång", closing));
  changes.rows.back().accounts = closing_accounts;
  for (auto const column : used) {
    Amount sum = opening[column];
    for (auto const& [text, amounts] : by_change) {
      sum += amounts[column];
    }
    if (sum != closing[column]) {
      changes.errors.push_back(std::format("Förändringar i eget kapital: {} opens at {} and changes to {}, but closes at {}",
                                           items[column]->text, format_amount(opening[column]), format_amount(sum),
                                           format_amount(closing[column])));
    }
  }
  return changes;
}

} // namespace cratchit::ar
