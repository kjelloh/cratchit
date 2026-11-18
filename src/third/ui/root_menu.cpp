#include "ui/root_menu.hpp"

#include "ui/help_topics.hpp"
#include "core/dot_folder.hpp"
#include "core/log.hpp"
#include "core/details_project.hpp"
#include "core/project.hpp"
#include "pyroceph/tui/error_state.hpp"
#include "ui/add_project_menu.hpp"
#include "ui/test_menu.hpp"
#include "ui/workspace_menu.hpp"

#include <format>
#include <string>
#include <utility>

namespace cratchit::ui {

namespace {

using pyroceph::tui::CodePoint;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

OnKeyResult cannot_open_workspace(std::string const& reason) {
  LOG_BUSINESS("cratchit kunde inte öppna samlingsmappen: {}", reason);
  return pyroceph::tui::make_error_state(std::format("cratchit kunde inte öppna samlingsmappen: {}", reason));
}

// True when folder holds a project (.cratchit/project.txt)
bool holds_project(std::filesystem::path const& folder) {
  std::error_code ec;
  return std::filesystem::is_regular_file(folder / ".cratchit" / "project.txt", ec);
}

class RootState : public State {
public:
  RootState(WorkspaceFolder workspace_folder, std::optional<std::filesystem::path> working_folder)
      : workspace_folder_(std::move(workspace_folder)), working_folder_(std::move(working_folder)) {}
  std::string name() const override { return "Cratchit"; }
  View view() const override {
    if (!working_folder_ || !holds_project(*working_folder_)) return {};
    auto const project = open_project(*working_folder_);
    if (!project) return {};
    return {std::format("Arbetsmappen har bokföringen {}. Du hittar den i samlingsmappen.", project->name)};
  }
  OptionsList options() const override {
    OptionsList options{{U'0', "Samlingsmapp"}};
    if (offers_here()) options.emplace_back(U'1', "Ny bokföring här");
    options.emplace_back(U'9', "Testmeny");
    return options;
  }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'0') return open();
    if (key == U'1' && offers_here()) return new_project_here();
    if (key == U'9') return make_test_menu();
    return nullptr;
  }
  std::string help_topic() const override { return std::string(help_topic::root); }

private:
  OnKeyResult open() const {
    if (!workspace_folder_) return cannot_open_workspace(workspace_folder_.error());
    auto workspace = open_workspace(*workspace_folder_, today_iso());
    if (!workspace) return cannot_open_workspace(workspace.error());
    for (auto const& entry : workspace->initiated) {
      LOG_BUSINESS("Skapade {} i {}", entry.file.generic_string(), entry.folder.string());
    }
    auto projects = list_projects(*workspace);
    if (!projects) return cannot_open_workspace(projects.error());
    LOG_BUSINESS("Öppnade samlingsmappen {} med {} projekt", workspace->folder.string(), projects->size());
    return make_workspace_menu(std::move(*workspace), std::move(*projects));
  }

  // 1 is offered while the working folder holds no project
  bool offers_here() const { return working_folder_ && !holds_project(*working_folder_); }

  OnKeyResult new_project_here() {
    auto const home = user_home();
    auto const workspace_path = workspace_folder_ ? std::optional(*workspace_folder_) : std::nullopt;
    if (auto const allowed = check_project_here(*working_folder_, workspace_path, home ? std::optional(*home) : std::nullopt);
        !allowed) {
      LOG_BUSINESS("Ingen ny bokföring i {}: {}", working_folder_->string(), allowed.error());
      return pyroceph::tui::make_error_state(std::format("cratchit skapar ingen bokföring här. {}", allowed.error()));
    }
    // The workspace remembers the new project
    if (!workspace_folder_) return cannot_open_workspace(workspace_folder_.error());
    auto workspace = open_workspace(*workspace_folder_, today_iso());
    if (!workspace) return cannot_open_workspace(workspace.error());
    return make_new_project_here(std::move(*workspace), *working_folder_, this);
  }

  WorkspaceFolder workspace_folder_;
  std::optional<std::filesystem::path> working_folder_; // where cratchit started, when known
};

} // namespace

std::unique_ptr<State> make_root_menu(WorkspaceFolder workspace_folder,
                                      std::optional<std::filesystem::path> working_folder) {
  return std::make_unique<RootState>(std::move(workspace_folder), std::move(working_folder));
}

} // namespace cratchit::ui
