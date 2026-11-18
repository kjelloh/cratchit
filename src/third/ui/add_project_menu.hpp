#pragma once

#include "core/dot_folder.hpp"
#include "pyroceph/tui/state.hpp"

#include <filesystem>
#include <memory>

namespace cratchit::ui {

// The key that adds to the list a state shows, e.g. a project to the workspace
inline constexpr pyroceph::tui::CodePoint add_key = U'+';

// Adds a project to the workspace, named "Lägg till" in the path. The options
// are short; the help text says more:
// * 1 "Befintlig" adds an existing project that the workspace does not list,
//   e.g. moved, copied or from another computer (see make_open_existing_project).
// * 2 "Ny" creates a new one, 1 from an SIE file ("Från SIE", see
//   sie_project_menu.hpp) or 2 from name, organisation number and fiscal year
//   ("Från uppgifter", see details_project_menu.hpp).
// No input. Reached by + from the workspace. start_folder is where the user
// starts typing the path of an existing project or of an SIE file. origin is
// the state the user started from (the workspace), which the TUI goes back to
// once a project is added or created; nullptr to stay.
std::unique_ptr<pyroceph::tui::State> make_add_project_menu(Workspace workspace, std::filesystem::path start_folder,
                                                            pyroceph::tui::State const* origin = nullptr);

// Creates a new project in folder itself (the working folder, see
// check_project_here), named "Ny bokföring här" in the path: as 2 "Ny" above,
// 1 "Från SIE" with the path starting in folder and 2 "Från uppgifter", but
// the project is made in folder's .cratchit instead of in a new folder (see
// make_project_from_sie and make_project_here). The TUI goes back to origin
// once it is created.
std::unique_ptr<pyroceph::tui::State> make_new_project_here(Workspace workspace, std::filesystem::path folder,
                                                           pyroceph::tui::State const* origin = nullptr);

// Adds an existing project, named "Befintlig" in the path. The user types the
// path of the project folder, starting from start_folder, and Tab completes
// folder names. Enter checks the project there (see open_project), makes the
// workspace remember it (see remember_project; a moved project is remembered
// at its new place), says so ("Samlingsmappen kommer ihåg …", "… kommer redan
// ihåg …" or "… kommer ihåg … på det nya stället.") and the TUI goes back to
// origin (State::back_to), the workspace that lists it. Without origin it
// stays. When it cannot be checked or remembered the reason is shown and the
// path is kept, to be corrected. No options.
std::unique_ptr<pyroceph::tui::State> make_open_existing_project(Workspace workspace, std::filesystem::path start_folder,
                                                                pyroceph::tui::State const* origin = nullptr);

} // namespace cratchit::ui
