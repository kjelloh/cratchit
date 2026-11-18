#include "core/skv/vat_return/steps.hpp"

#include "core/skv/accounts.hpp"
#include "core/skv/vat_return/box.hpp"

#include <algorithm>

namespace cratchit::skv::vat_return {

namespace {

enum class Kind { other, clearing, charge };

Kind kind_of(books::Voucher const& voucher) {
  bool clearing_account = false;
  bool vat_account = false;
  bool tax_account_posted = false;
  for (auto const& posting : voucher.postings) {
    if (posting.kind == books::Posting::Kind::removed) {
      continue;
    }
    clearing_account = clearing_account || is_vat_clearing_account(posting.account);
    vat_account = vat_account || is_vat_account(posting.account);
    tax_account_posted = tax_account_posted || posting.account == tax_account;
  }
  if (clearing_account && vat_account) {
    return Kind::clearing;
  }
  if (clearing_account && tax_account_posted) {
    return Kind::charge;
  }
  return Kind::other;
}

// The sum of the voucher's counted postings on accounts that pass
template <typename Pass>
Amount sum_of(books::Voucher const& voucher, Pass pass) {
  Amount sum = 0;
  for (auto const& posting : voucher.postings) {
    if (posting.kind != books::Posting::Kind::removed && pass(posting.account)) {
      sum += posting.amount;
    }
  }
  return sum;
}

bool holds(Period const& period, Date date) {
  return period.first <= date && date <= period.last;
}

} // namespace

Steps steps_of(books::Books const& books, std::vector<Period> const& periods) {
  Steps steps;
  for (auto const& period : periods) {
    steps.periods.push_back({period, 0, 0, {}, 0, {}});
  }
  std::vector<VoucherPosting> charges;
  for (auto const& year : books.years) {
    for (auto const& voucher : year.vouchers) {
      auto const date = voucher.event.date;
      auto const kind = kind_of(voucher);
      if (kind == Kind::charge) {
        // As on the VAT accounts: paying debits the clearing account, so the charge is its negation
        charges.push_back({year.start, voucher.series, voucher.number, date,
                           -sum_of(voucher, [](std::string_view a) { return is_vat_clearing_account(a); })});
        continue;
      }
      auto const in = std::ranges::find_if(steps.periods, [&](PeriodSteps const& p) { return holds(p.period, date); });
      if (in == steps.periods.end()) {
        continue;
      }
      if (kind == Kind::clearing) {
        auto const amount = sum_of(voucher, [](std::string_view a) { return is_vat_clearing_account(a); });
        in->cleared += amount;
        in->clearings.push_back({year.start, voucher.series, voucher.number, date, amount});
      } else {
        in->period_vat += sum_of(voucher, [](std::string_view a) { return is_vat_account(a); });
      }
    }
  }
  std::ranges::stable_sort(charges, {}, &VoucherPosting::date);
  for (auto const& charge : charges) {
    auto const to = std::ranges::find_if(steps.periods, [&](PeriodSteps const& p) {
      return !p.clearings.empty() && p.charges.empty() && p.cleared == charge.amount && p.period.first <= charge.date;
    });
    if (to == steps.periods.end()) {
      steps.unmatched_charges.push_back(charge);
    } else {
      to->charged = charge.amount;
      to->charges.push_back(charge);
    }
  }
  return steps;
}

} // namespace cratchit::skv::vat_return
