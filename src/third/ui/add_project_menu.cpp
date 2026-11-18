#include "ui/add_project_menu.hpp"

#include "ui/help_topics.hpp"
#include "core/log.hpp"
#include "core/project.hpp"
#include "pyroceph/tui/path_completion.hpp"
#include "ui/details_project_menu.hpp"
#include "ui/sie_project_menu.hpp"

#include <algorithm>
#include <format>
#include <optional>
#include <string>
#include <utility>

namespace cratchit::ui {

namespace {

using pyroceph::tui::Accepted;
using pyroceph::tui::CodePoint;
using pyroceph::tui::Completion;
using pyroceph::tui::InputResult;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

// The captions, used both in the menu and for the states they lead to
constexpr char const* open_existing_caption = "Befintlig";
constexpr char const* new_project_caption = "Ny";
constexpr char const* new_here_caption = "Ny bokföring här";
constexpr char const* from_sie_caption = "Från SIE";
constexpr char const* from_details_caption = "Från uppgifter";

// Creates a new project, from an SIE file or from the details the user enters
// (name, organisation number and fiscal year).
class NewProjectState : public State {
public:
  NewProjectState(Workspace workspace, std::filesystem::path start_folder, State const* origin,
                  std::optional<std::filesystem::path> here = std::nullopt)
      : workspace_(std::move(workspace)), start_folder_(std::move(start_folder)), origin_(origin), here_(std::move(here)) {}
  std::string name() const override { return here_ ? new_here_caption : new_project_caption; }
  View view() const override {
    if (here_) return {std::format("Bokföringen hamnar i {}.", here_->string())};
    return {};
  }
  OptionsList options() const override { return {{U'1', from_sie_caption}, {U'2', from_details_caption}}; }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'1') return make_project_from_sie(workspace_, start_folder_, origin_, here_);
    if (key == U'2') return here_ ? make_project_here(workspace_, *here_, origin_) : make_project_from_details(workspace_, origin_);
    return nullptr;
  }
  std::string help_topic() const override { return std::string(here_ ? help_topic::here : help_topic::new_project); }

private:
  Workspace workspace_;
  std::filesystem::path start_folder_;
  State const* origin_;
  std::optional<std::filesystem::path> here_; // the working folder to create the project in, else a new folder
};

class OpenExistingState : public State {
public:
  OpenExistingState(Workspace workspace, std::filesystem::path start_folder, State const* origin)
      : workspace_(std::move(workspace)), start_folder_(std::move(start_folder)), origin_(origin) {}
  std::string name() const override { return open_existing_caption; }
  View view() const override { return {"Skriv sökvägen till mappen med bokföringen och tryck Enter. Tab fyller i mappnamn."}; }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::string help_topic() const override { return std::string(help_topic::open_existing); }
  std::u32string start_input() const override { return pyroceph::tui::path_input_of(start_folder_); }
  Completion complete(std::u32string_view text) const override { return pyroceph::tui::complete_path(text); }
  InputResult on_input(std::u32string_view text) override {
    auto const folder = pyroceph::tui::path_of(text);
    auto project = open_project(folder);
    if (!project) {
      LOG_BUSINESS("Kunde inte öppna bokföringen i {}: {}", folder.string(), project.error());
      return std::unexpected(std::format("cratchit kunde inte öppna bokföringen: {}", project.error()));
    }
    // Where the workspace remembered it before, if it did
    std::optional<std::filesystem::path> before;
    if (auto const listed = list_projects(workspace_)) {
      auto const found = std::ranges::find(*listed, project->uuid, &ListedProject::uuid);
      if (found != listed->end()) before = found->folder;
    }
    if (auto const remembered = remember_project(workspace_, *project); !remembered) {
      LOG_BUSINESS("Samlingsmappen kunde inte komma ihåg projektet: {}", remembered.error());
      return std::unexpected(std::format("cratchit kunde inte skriva in bokföringen i samlingsmappen: {}", remembered.error()));
    }
    LOG_BUSINESS("Samlingsmappen {} kommer ihåg projektet {} ({}) i {}", workspace_.folder.string(), project->name,
                 project->uuid, project->folder.string());
    added_ = true;
    if (!before) return Accepted(std::format("Samlingsmappen kommer ihåg {}.", project->name));
    if (before->lexically_normal() == project->folder.lexically_normal()) {
      return Accepted(std::format("Samlingsmappen kommer redan ihåg {}.", project->name));
    }
    return Accepted(std::format("Samlingsmappen kommer ihåg {} på det nya stället.", project->name));
  }
  // Back to where the user started, the workspace, which now lists the project
  State const* back_to() const override { return added_ ? origin_ : nullptr; }

private:
  Workspace workspace_;
  std::filesystem::path start_folder_;
  State const* origin_;
  bool added_ = false;
};

class AddProjectState : public State {
public:
  AddProjectState(Workspace workspace, std::filesystem::path start_folder, State const* origin)
      : workspace_(std::move(workspace)), start_folder_(std::move(start_folder)), origin_(origin) {}
  std::string name() const override { return "Lägg till"; }
  View view() const override { return {}; }
  OptionsList options() const override { return {{U'1', open_existing_caption}, {U'2', new_project_caption}}; }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'1') return make_open_existing_project(workspace_, start_folder_, origin_);
    if (key == U'2') return std::make_unique<NewProjectState>(workspace_, start_folder_, origin_);
    return nullptr;
  }
  std::string help_topic() const override { return std::string(help_topic::add_project); }

private:
  Workspace workspace_;
  std::filesystem::path start_folder_;
  State const* origin_;
};

} // namespace

std::unique_ptr<State> make_add_project_menu(Workspace workspace, std::filesystem::path start_folder,
                                             State const* origin) {
  return std::make_unique<AddProjectState>(std::move(workspace), std::move(start_folder), origin);
}

std::unique_ptr<State> make_open_existing_project(Workspace workspace, std::filesystem::path start_folder,
                                                  State const* origin) {
  return std::make_unique<OpenExistingState>(std::move(workspace), std::move(start_folder), origin);
}

std::unique_ptr<State> make_new_project_here(Workspace workspace, std::filesystem::path folder, State const* origin) {
  auto start = folder;
  return std::make_unique<NewProjectState>(std::move(workspace), std::move(start), origin, std::move(folder));
}

} // namespace cratchit::ui
