#include "core/ar/trace.hpp"

#include <algorithm>
#include <charconv>
#include <map>
#include <set>
#include <tuple>

namespace cratchit::ar {

namespace {

int number_of(std::string_view number) {
  int value = 0;
  std::from_chars(number.data(), number.data() + number.size(), value);
  return value;
}

bool earlier(sie::Voucher const* left, sie::Voucher const* right) {
  return std::tuple(left->date, left->series, number_of(left->number)) <
         std::tuple(right->date, right->series, number_of(right->number));
}

} // namespace

std::string account_name(sie::Document const& document, std::string_view account) {
  auto const found = std::ranges::find(document.accounts, account, &sie::Account::number);
  return found == document.accounts.end() ? "" : found->name;
}

std::vector<TracedLine> trace_statement(Statement const& statement, sie::Document const& document) {
  std::size_t const years = statement.lines.empty() ? 0 : statement.lines.front().amounts.size();
  // Every account the file has an amount, opening balance or posting for
  std::set<std::string> accounts;
  for (auto const& balance : document.opening) {
    accounts.insert(balance.account);
  }
  for (auto const& balance : document.closing) {
    accounts.insert(balance.account);
  }
  for (auto const& result : document.results) {
    accounts.insert(result.account);
  }
  std::map<std::string, std::vector<Posting>> postings;
  for (auto const& voucher : document.vouchers) {
    for (auto const& transaction : voucher.transactions) {
      if (transaction.kind != sie::Transaction::Kind::removed) {
        accounts.insert(transaction.account);
        postings[transaction.account].push_back({&voucher, transaction.amount});
      }
    }
  }
  for (auto& [account, list] : postings) {
    std::ranges::stable_sort(list, earlier, &Posting::voucher);
  }
  std::vector<TracedLine> traced;
  for (auto const& line : statement.lines) {
    if (line.kind != LineKind::item || !line.shown || line.in_notes) {
      continue;
    }
    TracedLine out{&line, {}};
    for (auto const& account : accounts) {
      if (!line.account_set.contains(account)) {
        continue;
      }
      TracedAccount traced_account{account, account_name(document, account), std::vector<Amount>(years, 0), std::nullopt, {}};
      bool const is_balance = account.starts_with('1') || account.starts_with('2');
      for (std::size_t year = 0; year < years; ++year) {
        auto const& source = is_balance ? document.closing : document.results;
        for (auto const& balance : source) {
          if (balance.year == -static_cast<int>(year) && balance.account == account) {
            traced_account.amounts[year] += balance.amount;
          }
        }
      }
      if (is_balance) {
        Amount opening = 0;
        for (auto const& balance : document.opening) {
          if (balance.year == 0 && balance.account == account) {
            opening += balance.amount;
          }
        }
        traced_account.opening = opening;
      }
      if (auto const found = postings.find(account); found != postings.end()) {
        traced_account.postings = found->second;
      }
      bool const any = std::ranges::any_of(traced_account.amounts, [](Amount amount) { return amount != 0; }) ||
                       traced_account.opening.value_or(0) != 0 || !traced_account.postings.empty();
      if (any) {
        out.accounts.push_back(std::move(traced_account));
      }
    }
    traced.push_back(std::move(out));
  }
  return traced;
}

} // namespace cratchit::ar
