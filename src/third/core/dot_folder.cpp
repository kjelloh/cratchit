#include "core/dot_folder.hpp"

#include "core/path_value.hpp"

#include <cstdint>
#include <ctime>
#include <format>
#include <fstream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shlobj.h>
#else
#include <cstdlib>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

namespace cratchit {

namespace fs = std::filesystem;

namespace {

std::expected<void, std::string> write_new_file(fs::path const& file, std::string_view text) {
  std::ofstream out(file);
  out << text;
  if (!out) {
    return std::unexpected(std::format("Kan inte skriva {}", file.string()));
  }
  return {};
}

// Makes sure folder is a folder: true when it was created, false when it was
// already there. Only the folder itself is created, not its parents. An error
// other than "not found" (e.g. access denied) is reported, not taken as missing.
std::expected<bool, std::string> ensure_folder(fs::path const& folder) {
  std::error_code ec;
  auto const status = fs::status(folder, ec);
  if (ec && ec != std::errc::no_such_file_or_directory) {
    return std::unexpected(std::format("Kan inte nå {}: {}", folder.string(), ec.message()));
  }
  if (fs::exists(status)) {
    if (!fs::is_directory(status)) {
      return std::unexpected(std::format("{} finns men är inte en mapp", folder.string()));
    }
    return false;
  }
  if (!fs::create_directory(folder, ec)) {
    return std::unexpected(std::format("Kan inte skapa {}: {}", folder.string(), ec.message()));
  }
  return true;
}

#ifdef _WIN32
constexpr std::string_view user_home_variable = "USERPROFILE";

// Wide, so that a folder name is read correctly whatever the ANSI code page holds
std::optional<fs::path> environment_user_home() {
  auto const size = GetEnvironmentVariableW(L"USERPROFILE", nullptr, 0);
  if (size == 0) {
    return std::nullopt;
  }
  std::wstring value(size, L'\0');
  value.resize(GetEnvironmentVariableW(L"USERPROFILE", value.data(), size));
  return fs::path(value);
}
#else
constexpr std::string_view user_home_variable = "HOME";

std::optional<fs::path> environment_user_home() {
  auto const* value = std::getenv("HOME");
  if (value == nullptr) {
    return std::nullopt;
  }
  return fs::path(value);
}
#endif

// Where the running program file is, as the platform tells it. Not yet
// resolved: it may be a link.
#if defined(_WIN32)
std::expected<fs::path, std::string> program_file() {
  std::wstring buffer(MAX_PATH, L'\0');
  for (;;) {
    auto const size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (size == 0) return std::unexpected(std::string("GetModuleFileNameW misslyckades"));
    if (size < buffer.size()) {
      buffer.resize(size);
      return fs::path(buffer);
    }
    buffer.resize(buffer.size() * 2); // the path did not fit
  }
}
#elif defined(__APPLE__)
std::expected<fs::path, std::string> program_file() {
  std::uint32_t size = 0;
  _NSGetExecutablePath(nullptr, &size); // gives the size needed
  std::string buffer(size, '\0');
  if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
    return std::unexpected(std::string("_NSGetExecutablePath misslyckades"));
  }
  buffer.resize(std::char_traits<char>::length(buffer.c_str()));
  return fs::path(buffer);
}
#elif defined(__linux__)
std::expected<fs::path, std::string> program_file() {
  std::error_code ec;
  auto path = fs::read_symlink("/proc/self/exe", ec);
  if (ec) return std::unexpected(std::format("Kan inte läsa /proc/self/exe: {}", ec.message()));
  return path;
}
#else
#error "program_file() is not written for this platform"
#endif

} // namespace

std::expected<fs::path, std::string> program_path() {
  auto const file = program_file();
  if (!file) return std::unexpected(file.error());
  std::error_code ec;
  auto resolved = fs::canonical(*file, ec);
  if (ec) return std::unexpected(std::format("{}: {}", file->string(), ec.message()));
  return resolved;
}

std::expected<fs::path, std::string> program_folder() {
  auto const file = program_path();
  if (!file) return std::unexpected(std::format("Hittar inte programmets mapp: {}", file.error()));
  return file->parent_path();
}

std::expected<fs::path, std::string> choose_user_home(std::optional<fs::path> const& user_home) {
  if (!user_home || user_home->empty()) {
    return std::unexpected(std::format("Hittar inte hemkatalogen. {} är inte satt.", user_home_variable));
  }
  if (!user_home->is_absolute()) {
    return std::unexpected(std::format("{} måste vara en hel sökväg: {}", user_home_variable, user_home->string()));
  }
  std::error_code ec;
  if (!fs::is_directory(*user_home, ec)) {
    return std::unexpected(std::format("{} pekar på en mapp som inte finns: {}", user_home_variable, user_home->string()));
  }
  return *user_home;
}

std::expected<fs::path, std::string> user_home() { return choose_user_home(environment_user_home()); }

std::expected<Framework, std::string> open_framework(fs::path const& program_folder) {
  Framework result;
  result.file = program_folder / ".cratchit" / "framework.txt";
  std::error_code ec;
  if (!fs::is_regular_file(result.file, ec)) {
    return std::unexpected(std::format("{} saknas. Installationen av cratchit är inte hel.", result.file.string()));
  }
  auto const entries = read_path_values(result.file);
  if (!entries) return std::unexpected(entries.error());
  auto version = find_value(*entries, "version");
  if (!version || version->empty()) {
    return std::unexpected(std::format("{}: 'version' saknas", result.file.string()));
  }
  result.version = std::move(*version);
  return result;
}

std::expected<Runtime, std::string> open_runtime(fs::path const& home, std::string_view today) {
  Runtime result;
  auto const folder = home / ".cratchit";
  auto const runtime_name = fs::path(".cratchit") / "runtime.txt";
  result.file = home / runtime_name;

  std::error_code ec;
  if (fs::exists(folder, ec)) {
    if (!fs::is_directory(folder, ec)) {
      return std::unexpected(std::format("{} finns men är inte en mapp", folder.string()));
    }
  } else if (!fs::create_directory(folder, ec)) {
    return std::unexpected(std::format("Kan inte skapa {}: {}", folder.string(), ec.message()));
  }

  if (fs::is_regular_file(result.file, ec)) return result;
  if (fs::exists(result.file, ec)) {
    return std::unexpected(std::format("{} finns men är inte en fil", result.file.string()));
  }
  if (auto written = write_new_file(result.file, std::format("created = {}\n", today)); !written) {
    return std::unexpected(written.error());
  }
  result.initiated.push_back({home, runtime_name});
  return result;
}

std::expected<Workspace, std::string> open_workspace(fs::path const& folder, std::string_view today) {
  Workspace result;
  result.folder = folder;
  auto const workspace_name = fs::path(".cratchit") / "workspace.txt";
  result.file = folder / workspace_name;

  if (auto const made = ensure_folder(folder); !made) return std::unexpected(made.error());
  if (auto const made = ensure_folder(folder / ".cratchit"); !made) return std::unexpected(made.error());

  std::error_code ec;
  auto const status = fs::status(result.file, ec);
  if (ec && ec != std::errc::no_such_file_or_directory) {
    return std::unexpected(std::format("Kan inte nå {}: {}", result.file.string(), ec.message()));
  }
  if (fs::is_regular_file(status)) return result;
  if (fs::exists(status)) {
    return std::unexpected(std::format("{} finns men är inte en fil", result.file.string()));
  }
  if (auto written = write_new_file(result.file, std::format("created = {}\n", today)); !written) {
    return std::unexpected(written.error());
  }
  result.initiated.push_back({folder, workspace_name});
  return result;
}

#if defined(_WIN32)
std::expected<fs::path, std::string> documents_folder() {
  PWSTR path = nullptr;
  auto const hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &path);
  if (FAILED(hr)) {
    CoTaskMemFree(path);
    return std::unexpected(std::string("Hittar inte mappen Dokument (SHGetKnownFolderPath misslyckades)"));
  }
  fs::path result(path);
  CoTaskMemFree(path);
  return result;
}
std::expected<fs::path, std::string> downloads_folder() {
  PWSTR path = nullptr;
  auto const hr = SHGetKnownFolderPath(FOLDERID_Downloads, 0, nullptr, &path);
  if (FAILED(hr)) {
    CoTaskMemFree(path);
    return std::unexpected(std::string("Hittar inte mappen Hämtade filer (SHGetKnownFolderPath misslyckades)"));
  }
  fs::path result(path);
  CoTaskMemFree(path);
  return result;
}
#else
std::expected<fs::path, std::string> documents_folder() {
#if defined(__linux__)
  if (auto const* xdg = std::getenv("XDG_DOCUMENTS_DIR"); xdg != nullptr && fs::path(xdg).is_absolute()) {
    return fs::path(xdg);
  }
#endif
  auto const home = user_home();
  if (!home) return std::unexpected(std::format("Hittar inte mappen Dokument: {}", home.error()));
  return *home / "Documents";
}
std::expected<fs::path, std::string> downloads_folder() {
#if defined(__linux__)
  if (auto const* xdg = std::getenv("XDG_DOWNLOAD_DIR"); xdg != nullptr && fs::path(xdg).is_absolute()) {
    return fs::path(xdg);
  }
#endif
  auto const home = user_home();
  if (!home) return std::unexpected(std::format("Hittar inte mappen Hämtade filer: {}", home.error()));
  return *home / "Downloads";
}
#endif

std::expected<fs::path, std::string> default_workspace_folder() {
  return documents_folder().transform([](fs::path const& documents) { return documents / "cratchit"; });
}

std::optional<std::string> environment_variable(std::string_view name) {
  std::string const key(name);
#ifdef _WIN32
  auto const size = GetEnvironmentVariableA(key.c_str(), nullptr, 0);
  if (size == 0) return std::nullopt;
  std::string value(size, '\0');
  value.resize(GetEnvironmentVariableA(key.c_str(), value.data(), size));
  return value;
#else
  auto const* value = std::getenv(key.c_str());
  if (value == nullptr) return std::nullopt;
  return std::string(value);
#endif
}

namespace {

std::optional<std::string>& today_override() {
  static std::optional<std::string> today;
  return today;
}

} // namespace

void set_today(std::optional<std::string> today) { today_override() = std::move(today); }

std::string today_iso() {
  if (today_override()) return *today_override();
  std::time_t const now = std::time(nullptr);
  std::tm local{};
#ifdef _WIN32
  localtime_s(&local, &now);
#else
  localtime_r(&now, &local);
#endif
  return std::format("{:04}-{:02}-{:02}", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
}

} // namespace cratchit
