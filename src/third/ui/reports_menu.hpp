#pragma once

#include "core/books/books.hpp"
#include "core/fiscal_year.hpp"
#include "core/project.hpp"
#include "pyroceph/tui/state.hpp"

#include <expected>
#include <memory>
#include <string>
#include <vector>

namespace cratchit::ui {

// Rapporter: the reports of the fiscal year (core/books/report), opened with 3
// in the fiscal year. 1 Balansrapport, 2 Resultatrapport and 3
// Verifikationslista open the report (make_report_view); 4 Huvudbok asks for an
// account first (make_ledger_menu). The books are read when a report is chosen,
// so a report shows the books as they are then.
std::unique_ptr<pyroceph::tui::State> make_reports_menu(Project project, FiscalYear year);

// A report, whole: all its lines at once (the terminal scrolls), no paging,
// with an empty line before and after it for air on the screen
std::unique_ptr<pyroceph::tui::State> make_report_view(std::string name, std::vector<std::string> lines);

// Huvudbok: an account number typed and Enter opens the account's ledger in the
// fiscal year (make_report_view), and Enter alone the ledger of all accounts
// with rows or an opening balance in the year (a text field, so that an empty
// Enter reaches it). A number that is not an account of the books and has no
// rows in the year is refused with the reason, and the text is kept.
std::unique_ptr<pyroceph::tui::State> make_ledger_menu(Project project, FiscalYear year);

// The Utskrift of voucher in the fiscal year of the project's books (see
// books::voucher_document), headed by the project's name and organisation
// number as the reports are; error text when the books cannot be read or the
// year is not in them
std::expected<std::vector<std::string>, std::string> voucher_printout(Project const& project, FiscalYear const& year,
                                                                      books::Voucher const& voucher);

} // namespace cratchit::ui
