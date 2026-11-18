#pragma once

#include "core/dot_folder.hpp"
#include "pyroceph/tui/state.hpp"

#include <filesystem>
#include <memory>

namespace cratchit::ui {

// Creates a new project from details, named "Från uppgifter" in the path: a
// form whose options are its fields, each showing its value or what to give:
// 1 "Namn" (see new_project_folder), 2 "Organisationsnummer" (see parse_orgnr),
// 3 "Räkenskapsår", given as its first month (see parse_first_month), and
// 4 "Första årets längd" in months, 12 until changed (see
// parse_first_year_length). A digit leads to the field, a text field (see
// State::is_text_field) starting with the field's value: Enter takes a value
// the field accepts and goes back to the form; a value it refuses is shown
// why and kept; - goes back while the text is untouched, Esc empties it.
//
// When name, organisation number and first month are given, Enter in the
// form creates the project in the workspace folder (see
// create_project_from_details, the fiscal year from first_fiscal_year), without
// a chart of accounts yet. The workspace remembers it, and the TUI goes back to
// origin (State::back_to), e.g. the workspace that now lists it. Without origin
// the form stays and says that the project was created. A failure leads to the
// error state. No confirmation: the form shows everything before Enter.
std::unique_ptr<pyroceph::tui::State> make_project_from_details(Workspace workspace,
                                                                pyroceph::tui::State const* origin = nullptr);

// The same form, also named "Från uppgifter", for a project in folder
// itself (the working folder, see check_project_here), not in a new folder in
// the workspace: the name is any text that is not blank, and Enter creates the
// project with create_project_here. The workspace remembers it, as above.
std::unique_ptr<pyroceph::tui::State> make_project_here(Workspace workspace, std::filesystem::path folder,
                                                        pyroceph::tui::State const* origin = nullptr);

} // namespace cratchit::ui
