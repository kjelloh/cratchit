#pragma once

#include "core/dot_folder.hpp"
#include "pyroceph/tui/state.hpp"

#include <filesystem>
#include <memory>
#include <optional>

namespace cratchit::ui {

// Creates a new project from an SIE file, named "Från SIE" in the path. The
// user types the path of the SIE file, starting from start_folder; Tab
// completes folders and SIE files (.se, .si, .sie). Enter reads and plans the
// project (see plan_project_from_sie) and leads to "Bekräfta", which shows the
// plan and offers 1 "Skapa bokföringen". When the file cannot be used the reason
// is shown and the path is kept, to be corrected. No options.
//
// "Bekräfta" (J Ja, N Nej) shows the folder the books are made in: the SIE
// file's own folder, or here (beslut.md). J creates the project there (see
// create_project_from_sie): only a .cratchit folder is made, the whole file is
// imported and recorded in the inventory; the file stays where it is. The
// workspace remembers the project, and the TUI goes back to origin
// (State::back_to), where the user started, e.g. the workspace that now lists
// the project. Without origin "Bekräfta" stays and says that the project was
// created, offering nothing more. A failure leads to the error state.
//
// With here (the working folder, under "Ny bokföring här"), the project is made
// in that folder, wherever the SIE file lies, and once created the TUI goes on
// to the workspace.
std::unique_ptr<pyroceph::tui::State> make_project_from_sie(Workspace workspace, std::filesystem::path start_folder,
                                                            pyroceph::tui::State const* origin = nullptr,
                                                            std::optional<std::filesystem::path> here = std::nullopt);

} // namespace cratchit::ui
