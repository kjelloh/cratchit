#pragma once

#include "core/fiscal_year.hpp"
#include "core/project.hpp"
#include "pyroceph/tui/state.hpp"

#include <memory>

namespace cratchit::ui {

// A fiscal year of an opened project. Its name is the year's label (see
// label_of), so the path reads e.g. "Cratchit > Samlingsmapp > The ITfied AB > 2025/26".
// Its view shows the period, what the current zoom holds, what the last export
// or import did, and the zoom's vouchers and drafts, newest first (see
// year_view; for now the zoom is the whole year). Its
// options are 2 "Importera från SIE" (see sie_import_menu.hpp; the file must be
// for this year) and 1 "Exportera till SIE", a menu of its own:
// * 1 "Allt" writes the whole year with balances (see export_all) and goes back
//   to the year, which says what was written.
// * 2 "Skillnad" asks for an SIE file to compare with (a path from the project
//   folder, Tab completes folders and SIE files; see compare_with_sie) and shows
//   the vouchers only in cratchit and only in the file, warning when the file
//   was generated before today or has no date (the difference is only right
//   against a current export of the program exported to). When cratchit has vouchers
//   the file lacks, and the file none that cratchit lacks, 1 writes them without
//   balances (see export_difference) and goes back to the year.
// Files are written in the project folder and never overwrite one (see
// books::write_export). A failure leads to the error state. No input.
std::unique_ptr<pyroceph::tui::State> make_fiscal_year_menu(Project project, FiscalYear year);

} // namespace cratchit::ui
