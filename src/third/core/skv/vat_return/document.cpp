#include "core/skv/vat_return/document.hpp"

#include <algorithm>
#include <map>
#include <optional>
#include <utility>

namespace cratchit::skv::vat_return {

std::optional<Period> quarter_of(std::chrono::year year, unsigned quarter) {
  if (quarter < 1 || quarter > 4) {
    return std::nullopt;
  }
  auto const first = std::chrono::month{3 * quarter - 2};
  auto const last = std::chrono::month{3 * quarter};
  return Period{year / first / 1, year / last / std::chrono::last};
}

Period quarter_holding(Date date) {
  return *quarter_of(date.year(), (static_cast<unsigned>(date.month()) + 2) / 3);
}

std::vector<Period> quarters_ending_in(Date first, Date last) {
  std::vector<Period> quarters;
  for (auto year = first.year(); year <= last.year(); ++year) {
    for (unsigned quarter = 1; quarter <= 4; ++quarter) {
      auto const period = *quarter_of(year, quarter);
      if (first <= period.last && period.last <= last) {
        quarters.push_back(period);
      }
    }
  }
  return quarters;
}

namespace {

// A VAT clearing voucher, as left out of a return: counted postings on a VAT clearing account
// (2650, 1650) and on a VAT account, and on no other accounts than those, 3740 (öres- och
// kronutjämning), 1630 (skattekonto) and memo accounts (9000-9999, e.g. 9021 and 9099 turning a
// reverse charge base), as the checks allow them in a clearing. A purchase with its VAT against
// 2650 has other rows and counts (the checks flag it).
bool is_clearing(books::Voucher const& voucher) {
  bool clearing_account = false;
  bool vat_account = false;
  for (auto const& posting : voucher.postings) {
    if (posting.kind == books::Posting::Kind::removed) continue;
    bool const clears = is_vat_clearing_account(posting.account);
    bool const vat = is_vat_account(posting.account);
    bool const allowed = clears || vat || posting.account == "3740" || posting.account == "1630" ||
                         posting.account.starts_with('9');
    if (!allowed) return false;
    clearing_account = clearing_account || clears;
    vat_account = vat_account || vat;
  }
  return clearing_account && vat_account;
}

} // namespace

Document document_of(books::Books const& books, Period period, Clearings clearings) {
  // The period's own clearing: the latest day in the period with a clearing voucher (all of that day).
  // Earlier clearings in the period, e.g. a correction, count as the period's other vouchers do.
  std::optional<Date> last_clearing;
  if (clearings == Clearings::left_out) {
    for (auto const& year : books.years) {
      for (auto const& voucher : year.vouchers) {
        auto const date = voucher.event.date;
        if (period.first <= date && date <= period.last && is_clearing(voucher) &&
            (!last_clearing || *last_clearing < date)) {
          last_clearing = date;
        }
      }
    }
  }
  // box -> account -> its postings
  std::map<Box, std::map<std::string, std::vector<VoucherPosting>>> found;
  for (auto const& year : books.years) {
    for (auto const& voucher : year.vouchers) {
      auto const date = voucher.event.date;
      if (date < period.first || period.last < date) {
        continue;
      }
      if (last_clearing && date == *last_clearing && is_clearing(voucher)) {
        continue;
      }
      for (auto const& posting : voucher.postings) {
        if (posting.kind == books::Posting::Kind::removed) {
          continue;
        }
        if (auto const box = box_of(posting.account)) {
          found[*box][posting.account].push_back({year.start, voucher.series, voucher.number, date, posting.amount});
        }
      }
    }
  }

  Document document{period, {}};
  for (auto& [box, accounts] : found) {
    BoxAmount box_amount{box, 0, {}};
    for (auto& [account, postings] : accounts) {
      AccountAmount account_amount{account, 0, std::move(postings)};
      for (auto const& posting : account_amount.postings) {
        account_amount.amount += posting.amount;
      }
      box_amount.amount += account_amount.amount;
      box_amount.accounts.push_back(std::move(account_amount));
    }
    if (std::ranges::find(credit_boxes, box) != credit_boxes.end()) {
      box_amount.amount = -box_amount.amount; // the form takes credit as a positive figure
    }
    box_amount.amount = box_amount.amount / 100 * 100; // the form takes whole kronor, öre dropped
    document.boxes.push_back(std::move(box_amount));
  }
  return document;
}

Amount vat_to_pay(Document const& document) {
  Amount total = 0;
  for (auto const& box : document.boxes) {
    if (std::ranges::find(vat_to_pay_added, box.box) != vat_to_pay_added.end()) {
      total += box.amount;
    }
    if (std::ranges::find(vat_to_pay_subtracted, box.box) != vat_to_pay_subtracted.end()) {
      total -= box.amount;
    }
  }
  return total;
}

} // namespace cratchit::skv::vat_return
