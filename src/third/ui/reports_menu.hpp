#pragma once

#include "core/fiscal_year.hpp"
#include "core/project.hpp"
#include "pyroceph/tui/state.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace cratchit::ui {

// The lines a page of a report shows at most, for now
inline constexpr std::size_t report_page_rows = 20;

// Rapporter: the reports of the fiscal year (core/books/report), opened with 3
// in the fiscal year. 1 Balansrapport, 2 Resultatrapport and 3
// Verifikationslista open the report (make_report_view); 4 Huvudbok asks for an
// account first (make_ledger_menu). The books are read when a report is chosen,
// so a report shows the books as they are then.
std::unique_ptr<pyroceph::tui::State> make_reports_menu(Project project, FiscalYear year);

// A report a page at a time (see pyroceph::tui::Pager): a heading "<name>:
// rad 1–20 av 57", and "…" above and below the page when there are more
// lines. "<" turns a page back and ">" a page forward, in place; at either end
// the key does nothing.
std::unique_ptr<pyroceph::tui::State> make_report_view(std::string name, std::vector<std::string> lines);

// Huvudbok: an account number typed and Enter opens the account's ledger in the
// fiscal year (make_report_view). A number that is not an account of the books
// and has no rows in the year is refused with the reason, and the text is kept.
std::unique_ptr<pyroceph::tui::State> make_ledger_menu(Project project, FiscalYear year);

} // namespace cratchit::ui
