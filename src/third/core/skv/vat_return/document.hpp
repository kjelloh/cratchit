#pragma once

#include "core/amount.hpp"
#include "core/books/books.hpp"
#include "core/date.hpp"
#include "core/skv/vat_return/box.hpp"

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace cratchit::skv::vat_return {

// The days a VAT return reports, first and last included
struct Period {
  Date first;
  Date last;

  bool operator==(Period const&) const = default;
};

// A calendar quarter, 1 (January-March) to 4; nothing for any other quarter
std::optional<Period> quarter_of(std::chrono::year year, unsigned quarter);

// The calendar quarter a date is in
Period quarter_holding(Date date);

// The calendar quarters whose last day is in first - last (e.g. a fiscal year), oldest first.
// A quarter that starts before first still belongs: the VAT return of a quarter may span two fiscal years.
std::vector<Period> quarters_ending_in(Date first, Date last);

// A posting of a voucher on an account that reports in a box
struct VoucherPosting {
  Date fiscal_year; // the start of the voucher's fiscal year
  std::string series;
  std::string number;
  Date date; // the voucher's date
  Amount amount = 0;

  bool operator==(VoucherPosting const&) const = default;
};

// An account's part of a box: the sum of its postings
struct AccountAmount {
  std::string account;
  Amount amount = 0;
  std::vector<VoucherPosting> postings; // in book order

  bool operator==(AccountAmount const&) const = default;
};

// A box's amount as on the form: the sum of its accounts, the sign turned
// for credit_boxes, in whole kronor (öre dropped)
struct BoxAmount {
  Box box;
  Amount amount = 0;
  std::vector<AccountAmount> accounts; // by number

  bool operator==(BoxAmount const&) const = default;
};

// The VAT return of a period, as the books give it. Box amounts are the form's
// figures; account and voucher amounts are signed as in the books (debit
// positive, credit negative).
struct Document {
  Period period;
  std::vector<BoxAmount> boxes; // by box number; only boxes with postings

  bool operator==(Document const&) const = default;
};

// The VAT return of the vouchers dated in the period, from every fiscal year
// of the books: each counted posting (not removed ones) on an account that
// reports in a box (see box_of) adds to that box
// Whether the period's own VAT clearing counts: as in the books, so that a cleared
// period adds up to zero (take 3's screens), or left out, so that the return is the
// one the period's other vouchers make (the one filed, also after the period is
// cleared; cratchit --sie-to-vat). The period's own clearing is the clearing
// vouchers (postings only on VAT accounts, 2650/1650, 3740, 1630 and memo accounts
// 9000-9999) on the latest day in the period that has one; an earlier clearing in
// the period, e.g. a correction, counts.
enum class Clearings { counted, left_out };

Document document_of(books::Books const& books, Period period, Clearings clearings = Clearings::counted);

// Box 49 as the form adds it up from the box amounts (see vat_to_pay_added):
// positive is VAT to pay, negative VAT to get back
Amount vat_to_pay(Document const& document);

} // namespace cratchit::skv::vat_return
