#include "core/templates.hpp"

#include "core/amount.hpp"
#include "core/same_event.hpp"

#include <algorithm>
#include <set>
#include <tuple>
#include <utility>

namespace cratchit {

namespace {

bool counted(books::Posting const& posting) { return posting.kind != books::Posting::Kind::removed; }

int sign_of(Amount amount) { return amount < 0 ? -1 : 1; }

// The accounts and sides of a voucher's postings, to keep one voucher per pattern
std::set<std::pair<std::string, int>> pattern_of(books::Voucher const& voucher) {
  std::set<std::pair<std::string, int>> pattern;
  for (auto const& posting : voucher.postings) {
    if (counted(posting)) pattern.insert({posting.account, sign_of(posting.amount)});
  }
  return pattern;
}

} // namespace

std::vector<Template> find_templates(books::Books const& books, books::Draft const& draft, std::size_t max) {
  auto const turned = draft.event.amount < 0 ? -1 : 1;
  std::vector<books::Posting> own;
  for (auto const& posting : draft.postings) {
    if (counted(posting)) own.push_back(posting);
  }
  auto const text = compared_text(draft);
  std::vector<Template> found;
  for (auto const& year : books.years) {
    for (auto const& voucher : year.vouchers) {
      auto const rows = std::ranges::count_if(voucher.postings, counted);
      if (voucher.event.amount == 0 || rows < 2) continue;
      auto words = shared_words(text, voucher.event.heading);
      if (own.empty()) {
        if (words.empty()) continue;
      } else {
        auto const fits = std::ranges::all_of(own, [&](books::Posting const& posting) {
          return std::ranges::any_of(voucher.postings, [&](books::Posting const& row) {
            return counted(row) && row.account == posting.account &&
                   sign_of(row.amount) * turned == sign_of(posting.amount);
          });
        });
        if (!fits) continue;
      }
      found.push_back({voucher, std::move(words)});
    }
  }
  std::ranges::stable_sort(found, [](Template const& a, Template const& b) {
    if (a.words.size() != b.words.size()) return a.words.size() > b.words.size();
    return a.voucher.event.date > b.voucher.event.date;
  });
  std::vector<Template> best;
  std::vector<std::set<std::pair<std::string, int>>> patterns;
  for (auto& candidate : found) {
    if (best.size() == max) break;
    auto pattern = pattern_of(candidate.voucher);
    if (std::ranges::contains(patterns, pattern)) continue;
    patterns.push_back(std::move(pattern));
    best.push_back(std::move(candidate));
  }
  return best;
}

Applied apply_template(books::Draft const& draft, books::Voucher const& voucher,
                       std::vector<books::Account> const& accounts) {
  Applied applied{draft, {}};
  auto& result = applied.draft;
  auto const turned = draft.event.amount < 0;
  auto const amount = draft.event.amount != 0 ? (turned ? -draft.event.amount : draft.event.amount) : amount_of(draft);
  if (result.event.amount == 0) result.event.amount = amount;
  result.series = voucher.series;
  auto const has_account = [&](std::string const& account) {
    return std::ranges::any_of(draft.postings, [&](books::Posting const& p) { return counted(p) && p.account == account; });
  };
  std::size_t added = 0;
  for (auto const& row : voucher.postings) {
    if (!counted(row) || has_account(row.account)) continue;
    if (std::ranges::find(accounts, row.account, &books::Account::number) == accounts.end()) {
      applied.left_out.push_back(row.account);
      continue;
    }
    books::Posting posting;
    posting.account = row.account;
    posting.objects = row.objects;
    auto const value = amount != 0 && voucher.event.amount != 0 ? scaled(row.amount, amount, voucher.event.amount)
                                                                : row.amount;
    posting.amount = turned ? -value : value;
    result.postings.push_back(std::move(posting));
    ++added;
  }
  // The last row added gets what remains, so that the postings balance
  if (added > 0) {
    auto& last = result.postings.back();
    last.amount -= books::sum(result.postings);
  }
  return applied;
}

} // namespace cratchit
