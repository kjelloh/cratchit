#pragma once

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit {

// A file open_runtime created
struct Initiated {
  std::filesystem::path folder; // the folder holding .cratchit
  std::filesystem::path file;   // relative to folder, e.g. .cratchit/runtime.txt
};

// The framework that comes with the installation. Read only: cratchit never
// writes it. Later it also holds the rules for Swedish bookkeeping.
struct Framework {
  std::filesystem::path file; // <program folder>/.cratchit/framework.txt
  std::string version;        // the framework's `version` entry
};

// The user's own cratchit files, in the user's home folder
struct Runtime {
  std::filesystem::path file; // <home>/.cratchit/runtime.txt
  std::vector<Initiated> initiated;
};

// A workspace: the folder that holds the user's projects, each in a folder of
// its own below it. Named "Samlingsmapp" in the UI. Marked by
// <folder>/.cratchit/workspace.txt.
struct Workspace {
  std::filesystem::path folder;     // the workspace folder
  std::filesystem::path file;       // <folder>/.cratchit/workspace.txt
  std::vector<Initiated> initiated; // what open_workspace created
};

// The running program file, with links and `..` resolved. Error text when the
// platform cannot tell it.
std::expected<std::filesystem::path, std::string> program_path();

// The folder that holds the running program, with links and `..` resolved.
// Asks the platform (macOS _NSGetExecutablePath, Linux /proc/self/exe, Windows
// GetModuleFileNameW), since argv[0] is only what the starter passed, e.g. the
// bare name when started from PATH. Error text when it cannot be found.
std::expected<std::filesystem::path, std::string> program_folder();

// Opens <program_folder>/.cratchit/framework.txt and reads its version. Never
// creates or changes anything. Error text when the file is missing (the
// installation is not whole), malformed or has no `version`.
std::expected<Framework, std::string> open_framework(std::filesystem::path const& program_folder);

// Opens <home>/.cratchit/runtime.txt. A missing .cratchit or runtime.txt is a
// fresh start: created, with runtime.txt holding `created = <today>`. Existing
// files are never changed. Error text otherwise.
std::expected<Runtime, std::string> open_runtime(std::filesystem::path const& home, std::string_view today);

// Opens the workspace in folder. A missing folder is a fresh start: the folder
// itself (not its parents), .cratchit and workspace.txt holding
// `created = <today>` are created. Existing files are never changed. Error text
// otherwise, e.g. when the system denies access (macOS asks the user before a
// program may use Documents).
std::expected<Workspace, std::string> open_workspace(std::filesystem::path const& folder, std::string_view today);

// The user's documents folder, as the platform names it: Windows
// SHGetKnownFolderPath(FOLDERID_Documents) (follows a move to OneDrive), Linux
// XDG_DOCUMENTS_DIR when set, else <home>/Documents, macOS <home>/Documents.
// Error text when it cannot be found.
std::expected<std::filesystem::path, std::string> documents_folder();

// Where the workspace is by default: <documents folder>/cratchit
std::expected<std::filesystem::path, std::string> default_workspace_folder();

// The user's home folder. It must be a full path to an existing folder. Error
// text otherwise, or when it is not given or empty.
std::expected<std::filesystem::path, std::string> choose_user_home(std::optional<std::filesystem::path> const& user_home);

// choose_user_home from the environment: USERPROFILE on Windows, HOME elsewhere
std::expected<std::filesystem::path, std::string> user_home();

// The value of an environment variable, or nothing when it is not set. On
// Windows read with GetEnvironmentVariableA (MSVC warns that std::getenv is
// unsafe, C4996), elsewhere with std::getenv. For plain ASCII values, such as
// CRATCHIT_TODAY.
std::optional<std::string> environment_variable(std::string_view name);

// Today's local date as YYYY-MM-DD, or the date set with set_today
std::string today_iso();

// Makes today_iso give today (YYYY-MM-DD) instead of the clock's date, or the
// clock's date again with nothing. For tests, and for trying a date with the
// environment variable CRATCHIT_TODAY (see main).
void set_today(std::optional<std::string> today);

} // namespace cratchit
