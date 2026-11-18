#pragma once

#include "pyroceph/tui/state.hpp"
#include "ui/root_menu.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace cratchit::ui {

// Where cratchit starts. When folder (the working folder cratchit was started
// in) holds a project (.cratchit/project.txt), opens it as if the user had
// chosen 0 in the root and then the project: the navigator ends up in
// Cratchit > Samlingsmapp > <project>. The workspace in workspace_folder is made
// to remember the project (workspace.txt), so that it is listed when cratchit
// starts somewhere else. Only folder itself is looked in, not its parents.
//
// Leaves the navigator at the root when folder holds no project, or when the
// project or the workspace cannot be opened. Gives the lines to show the user
// before the prompt: none when folder holds no project.
std::vector<std::string> open_working_folder_project(pyroceph::tui::Navigator& navigator,
                                                     WorkspaceFolder const& workspace_folder,
                                                     std::filesystem::path const& folder);

} // namespace cratchit::ui
