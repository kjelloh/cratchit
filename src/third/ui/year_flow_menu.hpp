#pragma once

#include "core/fiscal_year.hpp"
#include "core/project.hpp"
#include "pyroceph/tui/state.hpp"

#include <memory>

namespace cratchit::ui {

// Årsflöde: the practical work with a fiscal year, as options in the order it is
// usually done over the year and after it: 0 Utkast, 1 Bokfört, 2 Momsrapport,
// 3 Bokslut, 4 Årsredovisning, 5 Årsstämma, 6 Rapport till Bolagsverket and
// 7 Deklaration till SKV. 0 leads to Utkast (make_draft_menu), 1 to Bokfört
// (make_booked_menu), 2 to Momsrapport (make_vat_return_menu), 3 to the Bokslut
// state (make_closing_menu), and 4 and 7 to how to make them from the command
// line (make_annual_report_menu, make_tax_return_menu); the others lead to Inte än
// (make_design_insufficiency_state) until they are built. Opened with 0 in the
// fiscal year of project. No input.
std::unique_ptr<pyroceph::tui::State> make_year_flow_menu(Project project, FiscalYear year);

// Bokslut: the work of closing the fiscal year. For now 0 Avstämning, which
// leads to Inte än. No input.
std::unique_ptr<pyroceph::tui::State> make_closing_menu();

} // namespace cratchit::ui
