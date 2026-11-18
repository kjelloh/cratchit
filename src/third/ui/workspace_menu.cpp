#include "ui/workspace_menu.hpp"

#include "ui/help_topics.hpp"
#include "core/log.hpp"
#include "ui/add_project_menu.hpp"
#include "ui/forget_project_menu.hpp"
#include "ui/project_menu.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <optional>
#include <string_view>
#include <system_error>
#include <tuple>
#include <cstddef>
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

// The width of UTF-8 text in columns, taking one column per code point
std::size_t width_of(std::string_view text) {
  return static_cast<std::size_t>(std::ranges::count_if(text, [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; }));
}

// text padded with spaces to width columns
std::string padded(std::string text, std::size_t width) {
  auto const used = width_of(text);
  if (used < width) text.append(width - used, ' ');
  return text;
}

// What is shown for a project, read from the project each time
struct Row {
  ListedProject listed;
  ProjectSummary summary;
  std::string folder; // short_folder_text
};

class WorkspaceState : public State {
public:
  WorkspaceState(Workspace workspace, std::vector<ListedProject> projects, std::string note)
      : workspace_(std::move(workspace)), projects_(std::move(projects)), note_(std::move(note)) {}
  std::string name() const override { return "Samlingsmapp"; }
  View view() const override {
    View lines;
    if (!note_.empty()) lines.push_back(note_);
    if (!workspace_.initiated.empty()) lines.push_back("cratchit skapade en ny samlingsmapp.");
    lines.push_back(workspace_.folder.string());
    if (projects_.size() > max_listed_projects) lines.push_back("Samlingsmappen har fler bokföringar än cratchit visar här.");
    auto const shown = rows();
    if (!shown.empty()) lines.push_back(std::string(header_indent) + line_of({"Namn", "Org.nr", "Skapad", "Mapp"}, widths_of(shown)));
    return lines;
  }
  OptionsList options() const override {
    OptionsList result;
    auto const shown = rows();
    auto const widths = widths_of(shown);
    for (std::size_t i = 0; i < shown.size(); ++i) result.push_back({key_of(i), caption_of(shown[i], widths)});
    result.push_back({add_key, "Lägg till bokföring"});
    return result;
  }
  OnKeyResult on_key(CodePoint key) override {
    auto const shown = rows();
    for (std::size_t i = 0; i < shown.size(); ++i) {
      if (key == key_of(i)) return open(shown[i].listed);
    }
    if (key == add_key) return make_add_project_menu(workspace_, workspace_.folder, this);
    return nullptr;
  }
  std::string help_topic() const override { return std::string(help_topic::workspace); }

  // A project may have been added on the way back here: read the list again
  void on_return() override {
    note_.clear(); // said once, when arriving
    auto projects = list_projects(workspace_);
    if (!projects) {
      LOG_BUSINESS("cratchit kunde inte läsa om samlingsmappen: {}", projects.error());
      return;
    }
    projects_ = std::move(*projects);
  }

private:
  // The header lines up with the options, which the TUI shows as "  <key> <caption>"
  static constexpr std::string_view header_indent = "    ";

  using Widths = std::array<std::size_t, 3>; // name, orgnr, created; the folder is last and not padded

  // The projects shown, up to 9, each read from its project now, ordered by name then folder
  std::vector<Row> rows() const {
    std::vector<Row> result;
    for (auto const& listed : projects_) {
      result.push_back({listed, describe_project(listed), short_folder_text(listed.folder, workspace_.folder, home_)});
    }
    std::ranges::sort(result, {}, [](Row const& row) { return std::tie(row.summary.name, row.listed.folder); });
    if (result.size() > max_listed_projects) result.resize(max_listed_projects);
    return result;
  }

  static Widths widths_of(std::vector<Row> const& rows) {
    Widths widths{width_of("Namn"), width_of("Org.nr"), width_of("Skapad")};
    for (auto const& row : rows) {
      widths[0] = std::max(widths[0], width_of(row.summary.name));
      widths[1] = std::max(widths[1], width_of(row.summary.orgnr));
      widths[2] = std::max(widths[2], width_of(row.summary.created));
    }
    return widths;
  }

  static std::string line_of(std::array<std::string, 4> const& columns, Widths const& widths) {
    return std::format("{}  {}  {}  {}", padded(columns[0], widths[0]), padded(columns[1], widths[1]),
                       padded(columns[2], widths[2]), columns[3]);
  }

  static std::string caption_of(Row const& row, Widths const& widths) {
    auto line = line_of({row.summary.name, row.summary.orgnr, row.summary.created, row.folder}, widths);
    if (row.summary.found == ProjectSummary::Found::missing) line += "  (finns inte längre)";
    if (row.summary.found == ProjectSummary::Found::unreadable) line += "  (går inte att öppna)";
    return line;
  }

  static CodePoint key_of(std::size_t index) { return static_cast<CodePoint>(U'1' + index); }

  // Opens the project, or offers to forget it when it is gone or cannot be opened
  OnKeyResult open(ListedProject const& listed) const {
    std::error_code ec;
    if (!std::filesystem::exists(listed.folder, ec)) {
      LOG_BUSINESS("Projektet i {} finns inte längre", listed.folder.string());
      return make_forget_project_menu(workspace_, listed, true, {}, this);
    }
    auto project = open_project(listed.folder, listed.uuid);
    if (!project) {
      LOG_BUSINESS("Kunde inte öppna projektet i {}: {}", listed.folder.string(), project.error());
      return make_forget_project_menu(workspace_, listed, false, project.error(), this);
    }
    LOG_BUSINESS("Öppnade projektet {} i {}", project->name, project->folder.string());
    return make_project_menu(std::move(*project));
  }

  Workspace workspace_;
  std::vector<ListedProject> projects_; // only where the projects are; the rest is read from them when shown
  std::string note_;                    // shown first until the user comes back from somewhere else
  std::optional<std::filesystem::path> home_ = [] -> std::optional<std::filesystem::path> {
    if (auto const home = user_home()) return *home;
    return std::nullopt;
  }();
};

} // namespace

std::unique_ptr<State> make_workspace_after_created(Workspace workspace, std::string const& name) {
  auto projects = list_projects(workspace);
  if (!projects) {
    LOG_BUSINESS("cratchit kunde inte läsa samlingsmappen: {}", projects.error());
    return nullptr;
  }
  return make_workspace_menu(std::move(workspace), std::move(*projects), std::format("cratchit skapade bokföringen {}.", name));
}

std::unique_ptr<State> make_workspace_menu(Workspace workspace, std::vector<ListedProject> projects, std::string note) {
  return std::make_unique<WorkspaceState>(std::move(workspace), std::move(projects), std::move(note));
}

} // namespace cratchit::ui
