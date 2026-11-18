#pragma once

#include "core/amount.hpp"
#include "core/books/books.hpp"
#include "core/skv/vat_return/document.hpp"

#include <vector>

// The steps the VAT of a period goes through in the books, told apart by the
// accounts posted to, not by the series:
// - period VAT: the postings on VAT accounts (see is_vat_account) of the vouchers dated in the period,
//   clearing vouchers left out
// - cleared: a clearing voucher, with postings on a VAT clearing account (see is_vat_clearing_account) and
//   on VAT accounts, moves the VAT of the period it is dated in to the clearing account
// - charged: a charge voucher, with postings on a VAT clearing account and on the tax account (skv::tax_account)
//   and none on VAT accounts, books what SKV charged or credited the tax account for a cleared period
// Payments between the tax account and the bank are not a step of a period: they settle the tax account's balance.
// Every amount is signed as on the VAT accounts: VAT to pay is negative (credit), VAT to get back positive (debit),
// so a period cleared and charged shows the same amount three times (cleared and charged in whole kronor).
namespace cratchit::skv::vat_return {

// The steps of one period
struct PeriodSteps {
  Period period;
  Amount period_vat = 0;
  Amount cleared = 0;
  std::vector<VoucherPosting> clearings; // dated in the period; amount as cleared
  Amount charged = 0;
  std::vector<VoucherPosting> charges; // matched to the period (at most one); amount as charged

  bool operator==(PeriodSteps const&) const = default;
};

struct Steps {
  std::vector<PeriodSteps> periods; // as given
  std::vector<VoucherPosting> unmatched_charges; // charges no period takes, by date

  bool operator==(Steps const&) const = default;
};

// The steps of the periods (oldest first, not overlapping) from every fiscal year of the books.
// Removed postings do not count. A charge is matched, in date order, to the oldest period that is
// cleared by the same amount, starts on or before the charge's date and has no charge yet; give every
// period of the books so that a charge finds its period.
Steps steps_of(books::Books const& books, std::vector<Period> const& periods);

} // namespace cratchit::skv::vat_return
