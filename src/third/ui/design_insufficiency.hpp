#pragma once

#include "pyroceph/tui/state.hpp"

#include <memory>
#include <source_location>
#include <string>
#include <string_view>

namespace cratchit::ui {

// A state for an option whose function is not there yet. option is the
// option's caption, what says what is missing as it reads after "infört" (both
// UTF-8), e.g. "Ny bokföring" and "att skapa en ny bokföring". Its name in the
// path is "Inte än (<option>)", its view "Utvecklaren av cratchit har inte
// infört <what> ännu.". It has no options and maps no keys, so - goes back, as
// from the error state.
//
// Logs a design insufficiency (LOG_DESIGN_INSUFFICIENCY) with the place it is
// called from, so the log names the file and line of each missing function the
// user has tried. Search the code for make_design_insufficiency_state to find
// the gaps.
std::unique_ptr<pyroceph::tui::State> make_design_insufficiency_state(
    std::string_view option, std::string_view what, std::source_location where = std::source_location::current());

} // namespace cratchit::ui
