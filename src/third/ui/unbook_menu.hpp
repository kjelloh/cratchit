#pragma once

#include "core/fiscal_year.hpp"
#include "core/project.hpp"
#include "pyroceph/tui/state.hpp"

#include <functional>
#include <memory>
#include <string>

namespace cratchit::ui {

// Gör till utkast (beslut 2026-11-18): the last voucher of each series in the
// fiscal year (last_vouchers), one line each: series, id, date, amount, text
// and the day it was registered, with a warning that a voucher that came from
// or went to another program (SIE) is still there. The user types a series and
// Enter, which makes its last voucher a draft at once (unbook_voucher; it can
// be undone by booking the draft again in the series, which gives the same
// number): reported through report, logged, then back to origin (back_to). A
// series without vouchers is refused. Opened with 0 in Bokfört and 5 in Utkast.
std::unique_ptr<pyroceph::tui::State> make_unbook_menu(Project project, FiscalYear year,
                                                       pyroceph::tui::State const* origin,
                                                       std::function<void(std::string)> report);

} // namespace cratchit::ui
