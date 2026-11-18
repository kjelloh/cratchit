#pragma once

#include "core/dot_folder.hpp"

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace cratchit {

// A project: the books of one juridical person, in a folder of its own that
// may be anywhere. Marked by <folder>/.cratchit/project.txt, in the format of
// take 3 (`created`, `uuid`, `name`, `orgnr`, ...). Only `uuid` and `name` are
// read for now; the other entries are left as they are.
struct Project {
  std::filesystem::path folder; // the project folder
  std::filesystem::path file;   // <folder>/.cratchit/project.txt
  std::string uuid;             // as in project.txt
  std::string name;             // the official name, e.g. "The ITfied AB"
};

// A project as the workspace remembers it: only where it is. Everything else
// is read from the project itself (see describe_project).
struct ListedProject {
  std::string uuid;
  std::filesystem::path folder; // full path, resolved from the workspace folder when relative
};

// The projects the workspace remembers: the `project.<uuid> = <folder>` entries
// of its workspace.txt (as in take 3), in the order there. A relative folder is
// read from the workspace folder. The folders are not searched. Error text when
// workspace.txt cannot be read, or an entry has no uuid or no folder.
std::expected<std::vector<ListedProject>, std::string> list_projects(Workspace const& workspace);

// What a listed project's own project.txt says, read anew each time
struct ProjectSummary {
  enum class Found {
    yes,        // project.txt read, with the listed uuid and a name
    missing,    // the folder is gone (removed or moved)
    unreadable, // the folder is there, but not a project with the listed uuid and a name
  };
  Found found = Found::yes;
  std::string name;    // `name`, or the folder name when not found
  std::string orgnr;   // `orgnr`, empty when not given or not found
  std::string created; // `created`, empty when not given or not found
};

// Reads the listed project's .cratchit/project.txt (see open_project). Never fails:
// a project that cannot be read is described as missing or unreadable.
ProjectSummary describe_project(ListedProject const& listed);

// How a project folder is shown: relative to the workspace folder when inside
// it, else starting with ~ when inside home, else in full
std::string short_folder_text(std::filesystem::path const& folder, std::filesystem::path const& workspace_folder,
                              std::optional<std::filesystem::path> const& home);

// Opens the project in folder: reads .cratchit/project.txt, which must hold a
// `name` and the `uuid` the workspace lists it with. Never creates or changes
// anything. Error text otherwise.
std::expected<Project, std::string> open_project(std::filesystem::path const& folder, std::string_view uuid);

// Opens the project in folder, whatever its uuid: as open_project, for a
// project the workspace does not list yet. folder must be a full path; it is
// normalised (no `.`, `..` or separator at the end). Error text otherwise.
std::expected<Project, std::string> open_project(std::filesystem::path const& folder);

// Makes the workspace remember the project: sets `project.<uuid> = <folder>`
// in workspace.txt (added last, or replacing the folder it had), keeping the
// other entries as they are. A folder has one project: another uuid remembered
// for the same folder is forgotten, and logged. Error text when workspace.txt cannot be read or written.
std::expected<void, std::string> remember_project(Workspace const& workspace, Project const& project);

// Makes the workspace forget the project with uuid: removes its
// `project.<uuid>` entry from workspace.txt, keeping the other entries as they
// are. Never touches the project's folder or files. Nothing to do when the
// workspace does not remember it. Error text when workspace.txt cannot be read
// or written.
std::expected<void, std::string> forget_project(Workspace const& workspace, std::string_view uuid);

} // namespace cratchit
