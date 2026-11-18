#include "core/project.hpp"

#include "core/log.hpp"
#include "core/path_value.hpp"

#include <algorithm>
#include <format>
#include <optional>
#include <system_error>

namespace cratchit {

namespace fs = std::filesystem;

namespace {

// The key prefix of a remembered project in workspace.txt
constexpr std::string_view project_prefix = "project.";

} // namespace

std::expected<std::vector<ListedProject>, std::string> list_projects(Workspace const& workspace) {
  auto const entries = read_path_values(workspace.file);
  if (!entries) return std::unexpected(entries.error());
  std::vector<ListedProject> result;
  for (auto const& entry : *entries) {
    if (!entry.path.starts_with(project_prefix)) continue;
    auto uuid = entry.path.substr(project_prefix.size());
    if (uuid.empty()) return std::unexpected(std::format("{}: '{}' saknar uuid", workspace.file.string(), entry.path));
    if (entry.value.empty()) {
      return std::unexpected(std::format("{}: '{}' saknar mapp", workspace.file.string(), entry.path));
    }
    fs::path folder(entry.value);
    if (folder.is_relative()) folder = workspace.folder / folder;
    result.push_back({std::move(uuid), std::move(folder)});
  }
  return result;
}

ProjectSummary describe_project(ListedProject const& listed) {
  ProjectSummary summary;
  summary.name = listed.folder.filename().string();
  std::error_code ec;
  if (!fs::exists(listed.folder, ec)) {
    summary.found = ProjectSummary::Found::missing;
    return summary;
  }
  auto const project = open_project(listed.folder, listed.uuid);
  if (!project) {
    summary.found = ProjectSummary::Found::unreadable;
    return summary;
  }
  summary.name = project->name;
  if (auto const entries = read_path_values(project->file)) {
    summary.orgnr = find_value(*entries, "orgnr").value_or("");
    summary.created = find_value(*entries, "created").value_or("");
  }
  return summary;
}

std::string short_folder_text(fs::path const& folder, fs::path const& workspace_folder,
                              std::optional<fs::path> const& home) {
  auto const inside = [](fs::path const& path, fs::path const& base) -> std::optional<fs::path> {
    auto const relative = path.lexically_normal().lexically_relative(base.lexically_normal());
    if (relative.empty() || relative == "." || *relative.begin() == "..") return std::nullopt;
    return relative;
  };
  if (auto const relative = inside(folder, workspace_folder)) return relative->string();
  if (home) {
    if (auto const relative = inside(folder, *home)) return (fs::path("~") / *relative).string();
  }
  return folder.string();
}

namespace {

// Reads project.txt in folder. The uuid must be expected_uuid, when given.
std::expected<Project, std::string> read_project(fs::path const& folder, std::optional<std::string_view> expected_uuid) {
  Project result;
  result.folder = folder;
  result.file = folder / ".cratchit" / "project.txt";
  std::error_code ec;
  if (!fs::is_regular_file(result.file, ec)) {
    return std::unexpected(std::format("{} saknas", result.file.string()));
  }
  auto const entries = read_path_values(result.file);
  if (!entries) return std::unexpected(entries.error());
  auto found_uuid = find_value(*entries, "uuid");
  if (!found_uuid || found_uuid->empty()) return std::unexpected(std::format("{}: 'uuid' saknas", result.file.string()));
  if (expected_uuid && *found_uuid != *expected_uuid) {
    return std::unexpected(std::format("{}: uuid {} är inte {} som samlingsmappen har för mappen", result.file.string(),
                                       *found_uuid, *expected_uuid));
  }
  auto name = find_value(*entries, "name");
  if (!name || name->empty()) return std::unexpected(std::format("{}: 'name' saknas", result.file.string()));
  result.uuid = std::move(*found_uuid);
  result.name = std::move(*name);
  return result;
}

} // namespace

std::expected<Project, std::string> open_project(fs::path const& folder, std::string_view uuid) {
  return read_project(folder, uuid);
}

std::expected<Project, std::string> open_project(fs::path const& folder) {
  if (!folder.is_absolute()) return std::unexpected(std::format("{} är inte en hel sökväg", folder.string()));
  auto normal = folder.lexically_normal();
  if (!normal.has_filename() && normal.has_relative_path()) normal = normal.parent_path();
  return read_project(normal, std::nullopt);
}

std::expected<void, std::string> forget_project(Workspace const& workspace, std::string_view uuid) {
  auto entries = read_path_values(workspace.file);
  if (!entries) return std::unexpected(entries.error());
  auto const path = std::string(project_prefix) + std::string(uuid);
  auto const removed = std::erase_if(*entries, [&path](PathValue const& entry) { return entry.path == path; });
  if (removed == 0) return {};
  return write_path_values(workspace.file, *entries);
}

std::expected<void, std::string> remember_project(Workspace const& workspace, Project const& project) {
  auto entries = read_path_values(workspace.file);
  if (!entries) return std::unexpected(entries.error());
  // A folder has one project (beslut.md): forget another uuid remembered for the same folder
  auto const key = std::string(project_prefix) + project.uuid;
  auto const same_folder = [&](PathValue const& entry) {
    if (!entry.path.starts_with(project_prefix) || entry.path == key) return false;
    auto folder = fs::path(entry.value);
    if (folder.is_relative()) folder = workspace.folder / folder;
    auto const normal = [](fs::path p) {
      p = p.lexically_normal();
      return !p.has_filename() && p.has_relative_path() ? p.parent_path() : p;
    };
    std::error_code ec;
    if (normal(folder) == normal(project.folder)) return true;
    return fs::equivalent(folder, project.folder, ec) && !ec;
  };
  for (auto const& entry : *entries) {
    if (same_folder(entry)) {
      LOG_BUSINESS("Samlingsmappen glömmer {} i {}: mappen har nu bokföringen {}", entry.path, entry.value, project.uuid);
    }
  }
  std::erase_if(*entries, same_folder);
  set_value(*entries, std::string(project_prefix) + project.uuid, project.folder.string());
  return write_path_values(workspace.file, *entries);
}

} // namespace cratchit
