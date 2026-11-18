#pragma once

#include "core/fiscal_year.hpp"
#include "core/project.hpp"
#include "pyroceph/tui/state.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace cratchit::ui {

// Imports an SIE file into a fiscal year of the project, named "Importera från
// SIE" in the path. The user types the path of the SIE file, starting in the
// project folder; Tab completes folders and SIE files. Enter plans the import
// (see plan_sie_import; the file must be for year) and leads to "Bekräfta",
// which shows the file, the year and what it adds and changes, and offers
// 1 "Importera". When the file cannot be imported the reason is shown and the
// path is kept.
//
// "Bekräfta" 1 imports (see import_sie: the books, and the SIE file recorded in
// the inventory; the file stays where it is), gives report what was done and the year imported
// (its dates are the file's, which may differ from year's when it adjoins the
// books, see plan_sie_import), and the TUI goes back to origin
// (State::back_to), the fiscal year. A failure leads to the error state.
std::unique_ptr<pyroceph::tui::State> make_sie_import_menu(Project project, FiscalYear year,
                                                           pyroceph::tui::State const* origin,
                                                           std::function<void(std::vector<std::string>, FiscalYear)> report);

} // namespace cratchit::ui
