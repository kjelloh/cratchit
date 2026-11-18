#pragma once

#include "core/books/books.hpp"
#include "core/skv/vat_return/document.hpp"
#include "core/skv/vat_return/steps.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit::skv::vat_return {

// The vouchers of steps in a line, e.g. A 12, A 13
std::string vouchers_text(std::vector<VoucherPosting> const& vouchers);

// The column heading of steps_row: Period, Cleared, Charged
std::string steps_heading();

// The period VAT, cleared and charged amounts of a period in columns, then the vouchers that cleared and charged it
std::string steps_row(PeriodSteps const& steps);

// A quarter's name, e.g. 2026 Q3 for July - September 2026
std::string quarter_name(Period period);

// Report lines of a VAT return, headed by the period and the company: each box
// with its amount, each of its accounts with its sum and name, and the voucher
// postings behind it with the voucher text, box 49 (see vat_to_pay), then the period's steps (period VAT,
// cleared, charged), and box 49 as the form shows it. Amounts keep their sign as in the books. With so_far (today), the period is still running.
std::vector<std::string> report_of(books::Books const& books, Document const& document, PeriodSteps const& steps,
                                   std::string_view company, std::optional<Date> so_far = std::nullopt);

} // namespace cratchit::skv::vat_return
