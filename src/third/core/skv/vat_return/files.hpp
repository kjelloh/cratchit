#pragma once

#include "core/books/books.hpp"
#include "core/date.hpp"
#include "core/skv/vat_return/clearing.hpp"
#include "core/skv/vat_return/document.hpp"
#include "core/skv/vat_return/steps.hpp"

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit::skv::vat_return {

// The texts and files of a period's VAT return, shared by cratchit --sie-to-vat
// and Årsflöde > Momsrapport. The texts are in Swedish.

// "202609": the year and last month of the period, as eSKD names it
std::string eskd_period(Period const& period);

// The report of the period (report_of) followed by its checks (checks_of) and
// the line made_by, e.g. "Gjord 2026-11-18 av cratchit 0.9.1"
std::vector<std::string> report_with_checks(books::Books const& books, Document const& document,
                                            PeriodSteps const& steps, std::string_view company,
                                            std::optional<Date> so_far, std::string_view made_by);

// The suggested clearing voucher of the period, as text: its series, date,
// heading, amount and postings with the accounts' names; that there is nothing
// to clear; or, when steps has clearings, that the period is cleared already
std::vector<std::string> voucher_lines(books::Books const& books, std::optional<Clearing> const& clearing,
                                       PeriodSteps const& steps);

// Writes the lines to file, each ending with LF; error text when it cannot
std::expected<void, std::string> write_text(std::filesystem::path const& file, std::vector<std::string> const& lines);

} // namespace cratchit::skv::vat_return
