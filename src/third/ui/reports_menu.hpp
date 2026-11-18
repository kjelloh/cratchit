#pragma once

#include "core/books/books.hpp"
#include "core/dot_folder.hpp"
#include "core/fiscal_year.hpp"
#include "core/project.hpp"
#include "pyroceph/tui/state.hpp"

#include <expected>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace cratchit::ui {

// Finds a folder, e.g. downloads_folder (tests give one of their own)
using FindFolder = std::function<std::expected<std::filesystem::path, std::string>()>;

// Where a report shown whole may be saved (0 Spara som fil): in the downloads
// folder when it is there, else in fallback_folder (the project's folder), as
// <title>.txt (see save_document), e.g. "Test AB Balansrapport 2025-26"
struct SaveAs {
  std::filesystem::path fallback_folder;
  std::string title;
  FindFolder downloads = downloads_folder;
};

// Rapporter: the reports of the fiscal year (core/books/report), opened with 3
// in the fiscal year. 1 Balansrapport, 2 Resultatrapport and 3
// Verifikationslista open the report (make_report_view); 4 Huvudbok asks for an
// account first (make_ledger_menu). The books are read when a report is chosen,
// so a report shows the books as they are then. Each report may be saved,
// to downloads or else the project's folder.
std::unique_ptr<pyroceph::tui::State> make_reports_menu(Project project, FiscalYear year,
                                                        FindFolder downloads = downloads_folder);

// A report, whole: all its lines at once (the terminal scrolls), no paging,
// with an empty line before and after it for air on the screen. With save,
// 0 Spara som fil writes the report's lines (without the empty lines around
// them) to a file and tells below the report where it went, also when it went
// to the fallback folder for want of the downloads folder; the user stays.
std::unique_ptr<pyroceph::tui::State> make_report_view(std::string name, std::vector<std::string> lines,
                                                       std::optional<SaveAs> save = {});

// Huvudbok: an account number typed and Enter opens the account's ledger in the
// fiscal year (make_report_view), and Enter alone the ledger of all accounts
// with rows or an opening balance in the year (a text field, so that an empty
// Enter reaches it). A number that is not an account of the books and has no
// rows in the year is refused with the reason, and the text is kept.
std::unique_ptr<pyroceph::tui::State> make_ledger_menu(Project project, FiscalYear year,
                                                       FindFolder downloads = downloads_folder);

// The Utskrift of voucher in the fiscal year of the project's books (see
// books::voucher_document), headed by the project's name and organisation
// number as the reports are, shown whole and with 0 Spara som fil; error text
// when the books cannot be read or the year is not in them
std::expected<std::unique_ptr<pyroceph::tui::State>, std::string>
make_voucher_printout(Project const& project, FiscalYear const& year, books::Voucher const& voucher,
                      FindFolder downloads = downloads_folder);

} // namespace cratchit::ui
