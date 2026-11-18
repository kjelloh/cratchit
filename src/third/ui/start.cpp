#include "ui/start.hpp"

#include "core/date.hpp"
#include "core/dot_folder.hpp"
#include "core/log.hpp"
#include "core/project.hpp"
#include "ui/project_menu.hpp"
#include "ui/workspace_menu.hpp"

#include <algorithm>
#include <format>
#include <optional>
#include <system_error>
#include <utility>

namespace cratchit::ui {

namespace fs = std::filesystem;

namespace {

std::string not_opened(std::string const& reason) {
  LOG_BUSINESS("Kunde inte öppna bokföringen i arbetsmappen: {}", reason);
  return std::format("cratchit kunde inte öppna bokföringen i arbetsmappen: {}", reason);
}

} // namespace

std::vector<std::string> open_working_folder_project(pyroceph::tui::Navigator& navigator,
                                                     WorkspaceFolder const& workspace_folder,
                                                     fs::path const& folder) {
  std::error_code ec;
  if (!fs::is_regular_file(folder / ".cratchit" / "project.txt", ec)) return {};

  auto project = open_project(folder);
  if (!project) return {not_opened(project.error())};
  if (!workspace_folder) return {not_opened(workspace_folder.error())};
  auto workspace = open_workspace(*workspace_folder, today_iso());
  if (!workspace) return {not_opened(workspace.error())};
  for (auto const& entry : workspace->initiated) {
    LOG_BUSINESS("Skapade {} i {}", entry.file.generic_string(), entry.folder.string());
  }

  std::vector<std::string> lines{std::format("cratchit öppnade bokföringen i arbetsmappen: {}", project->name)};

  // Remembered by the workspace, so that it is listed when cratchit starts elsewhere
  std::optional<fs::path> before;
  if (auto const listed = list_projects(*workspace)) {
    auto const found = std::ranges::find(*listed, project->uuid, &ListedProject::uuid);
    if (found != listed->end()) before = found->folder;
  }
  if (auto const remembered = remember_project(*workspace, *project); !remembered) {
    LOG_BUSINESS("Samlingsmappen kunde inte komma ihåg projektet: {}", remembered.error());
    lines.push_back(std::format("cratchit kunde inte skriva in bokföringen i samlingsmappen: {}", remembered.error()));
  } else if (!before) {
    LOG_BUSINESS("Samlingsmappen {} kommer ihåg projektet {} ({}) i {}", workspace->folder.string(), project->name,
                 project->uuid, project->folder.string());
    lines.push_back(std::format("Samlingsmappen kommer ihåg {}.", project->name));
  } else if (before->lexically_normal() != project->folder.lexically_normal()) {
    LOG_BUSINESS("Samlingsmappen {} kommer ihåg projektet {} ({}) i {} i stället för {}", workspace->folder.string(),
                 project->name, project->uuid, project->folder.string(), before->string());
    lines.push_back(std::format("Samlingsmappen kommer ihåg {} här i stället för i {}.", project->name, before->string()));
  }

  auto projects = list_projects(*workspace);
  if (!projects) return {not_opened(projects.error())};
  LOG_BUSINESS("Öppnade projektet {} i arbetsmappen {}", project->name, project->folder.string());
  navigator.enter(make_workspace_menu(std::move(*workspace), std::move(*projects)));
  navigator.enter(make_project_menu(std::move(*project)));
  return lines;
}

} // namespace cratchit::ui
