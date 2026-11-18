#pragma once

#include "pyroceph/tui/state.hpp"

#include <expected>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace cratchit::ui {

// Where 0 opens the workspace, or why that place is not known (shown as the
// error when the user presses 0)
using WorkspaceFolder = std::expected<std::filesystem::path, std::string>;

// The root of cratchit's UI, named "Cratchit" in the prompt. 0 opens the
// workspace in workspace_folder (see workspace_menu.hpp), or leads to the error
// state when it cannot be opened. 9 leads to the test menu (see test_menu.hpp).
//
// working_folder is where cratchit was started. While it holds no project,
// 1 "Ny bokföring här" creates one in it (see make_new_project_here), or leads to
// the error state saying why not (see check_project_here); afterwards the TUI
// is back in the root. While it holds a project, the view names it.
std::unique_ptr<pyroceph::tui::State> make_root_menu(WorkspaceFolder workspace_folder,
                                                     std::optional<std::filesystem::path> working_folder = std::nullopt);

} // namespace cratchit::ui
