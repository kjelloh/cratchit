#pragma once

#include "core/fiscal_year.hpp"
#include "core/project.hpp"
#include "pyroceph/tui/state.hpp"

#include <cstddef>
#include <memory>

namespace cratchit::ui {

// The most fiscal years a project offers, as the keys 0..9
inline constexpr std::size_t max_listed_fiscal_years = 1 + max_older_fiscal_years; // the current year and the older ones

// An opened project. Its name is the project's name, so the path reads e.g.
// "Cratchit > Samlingsmapp > The ITfied AB". Its view shows the folder (and
// when there are no fiscal years, more than are offered, or they cannot be
// read). Its options are the fiscal years (see list_years_until with today):
// the current year (the one today is in) as 0 and the older ones as 1, 2, ...
// up to 9, like #RAR 0, -1, -2 in SIE without the minus, back to the oldest
// in the books. A year not booked yet is listed too, so a new year shows when
// it begins. Each tells what is booked: "bokfört: inget" or "bokfört: 37
// verifikationer". Each leads to its year (see
// fiscal_year_menu.hpp). The years are read again when the user comes back.
// No input. Reached by a digit from the workspace.
std::unique_ptr<pyroceph::tui::State> make_project_menu(Project project);

} // namespace cratchit::ui
