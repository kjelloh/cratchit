#pragma once

#include "core/books/books.hpp"
#include "core/skv/vat_return/box.hpp"
#include "core/skv/vat_return/document.hpp"

#include <optional>
#include <string>
#include <vector>

// Checks of the books behind a period's VAT return: what could make the return wrong.
// They only warn; each finding is one line for the user.
namespace cratchit::skv::vat_return {

// The VAT rate of an output VAT box in percent: 10, 30, 60 are 25 %, 11, 31, 61 are 12 %, 12, 32, 62 are 6 %
std::optional<int> rate_of(Box const& box);

// The findings of the period, from every fiscal year of the books, counted postings only:
// 1. Not fully booked: drafts dated in the period (in cratchit, events and statement entries not yet
//    booked are drafts; take 3 had them apart).
// 2. The VAT clearing accounts (2650, 1650) posted in a voucher whose other rows are not all on VAT accounts,
//    the tax account (1630), the rounding account (3740) or memo accounts (9000-9999): only a clearing or a
//    charge voucher posts them.
// 3. Reverse charge and import in pairs, per voucher: a purchase base (boxes 20-24, or 50) needs output VAT
//    (boxes 30-32, or 60-62) and the other way round; with one output VAT box its amount must be the base
//    times its rate, rounded towards zero to whole öre; and box 48 must deduct that output VAT in full.
// 4. A voucher on an account of a box, dated in the period, registered after the period's clearing voucher
//    (a voucher by rule 2 with rows on VAT accounts): the return made then lacks it.
// 5. Postings on accounts 2600-2699 that report in no box (2650 aside).
std::vector<std::string> checks_of(books::Books const& books, Period period);

} // namespace cratchit::skv::vat_return
