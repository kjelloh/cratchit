#pragma once

#include "core/dot_folder.hpp"
#include "core/project.hpp"
#include "pyroceph/tui/state.hpp"

#include <memory>
#include <string>

namespace cratchit::ui {

// A project the workspace remembers but that cannot be opened. Its name is the
// folder's name. Its view says where the project was and why it cannot be
// opened: the folder is gone (missing, see ProjectSummary), or it is not the
// remembered project (reason, e.g. from open_project). It offers 0 "Glöm",
// which makes the workspace forget the project (see forget_project, which never
// touches files) and then the TUI goes back to origin (State::back_to), the
// workspace, which no longer lists it. - goes back without forgetting. A
// failure leads to the error state. No input.
std::unique_ptr<pyroceph::tui::State> make_forget_project_menu(Workspace workspace, ListedProject listed, bool missing,
                                                               std::string reason, pyroceph::tui::State const* origin);

} // namespace cratchit::ui
