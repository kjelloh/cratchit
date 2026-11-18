#pragma once

#include "core/fiscal_year.hpp"
#include "core/project.hpp"
#include "pyroceph/tui/state.hpp"

#include <cstddef>
#include <memory>

namespace cratchit::ui {

// The lines of a quarter's report shown at a time; < and > turn a page
inline constexpr std::size_t vat_report_page_rows = 40;

// Årsflöde > Momsrapport (beslut 2026-11-18, take 3's VAT return in Swedish):
// the calendar quarters ending in the fiscal year, each with its VAT in three
// steps (period, avräknat, debiterat; see skv::vat_return::steps_of). The
// ended ones are options from 0, the latest first, "nästa att rapportera" on
// the one before today's quarter and the number of checks on those with any;
// the quarter running today is + ("hittills"). Charges dated in the year that
// no quarter takes are listed below. The books are read again when the user
// comes back.
//
// A quarter (its name, e.g. "2026 Q3"): the VAT return of its vouchers without
// its own VAT clearing (Clearings::left_out, as --sie-to-vat), report_of a page
// at a time (< and >), then its checks, the suggested clearing voucher (or that
// it is cleared, or that there is nothing to clear) and the eSKD file's fields.
//   0 Granska rutorna: every box of the form with its title and postings; a
//     box number and Enter shows the box against SKV's description, < and >
//     the box before and after.
//   1 Momstömning (an ended quarter with something to clear, not cleared): the
//     clearing voucher as a draft in series M, dated the quarter's last day,
//     then Bokför for it at once; - leaves it a draft. A draft with the same
//     heading and date is used again (replaced when the books changed it).
//   2 Skriv rapport och eSKD-fil: the folder moms_<first>_<last> in the
//     project's folder, with momsrapport.txt (report and checks) and, for an
//     ended quarter, verifikation.txt (the clearing) and the eSKD file
//     (moms_<YYYYMM>.eskd; never overwritten: the next is _2, _3, ...). What was
//     written is shown below the view.
std::unique_ptr<pyroceph::tui::State> make_vat_return_menu(Project project, FiscalYear year);

} // namespace cratchit::ui
