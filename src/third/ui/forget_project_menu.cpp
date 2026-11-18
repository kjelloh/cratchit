#include "ui/forget_project_menu.hpp"

#include "core/log.hpp"
#include "pyroceph/tui/error_state.hpp"
#include "ui/help_topics.hpp"

#include <format>
#include <utility>

namespace cratchit::ui {

namespace {

using pyroceph::tui::CodePoint;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

class ForgetProjectState : public State {
public:
  ForgetProjectState(Workspace workspace, ListedProject listed, bool missing, std::string reason, State const* origin)
      : workspace_(std::move(workspace)), listed_(std::move(listed)), missing_(missing), reason_(std::move(reason)),
        origin_(origin) {}
  std::string name() const override { return listed_.folder.filename().string(); }
  View view() const override {
    if (missing_) {
      return {std::format("Bokföringen fanns i {}, men mappen finns inte längre.", listed_.folder.string()),
              "Den kan vara borttagen eller flyttad. Har du flyttat den kan du lägga till den på det nya stället med + i "
              "samlingsmappen."};
    }
    return {std::format("Mappen {} finns, men cratchit kan inte öppna bokföringen där: {}", listed_.folder.string(), reason_)};
  }
  OptionsList options() const override {
    if (forgotten_) return {};
    return {{U'0', "Glöm"}};
  }
  OnKeyResult on_key(CodePoint key) override {
    if (forgotten_ || key != U'0') return nullptr;
    if (auto const forgotten = forget_project(workspace_, listed_.uuid); !forgotten) {
      LOG_BUSINESS("Samlingsmappen kunde inte glömma projektet {}: {}", listed_.uuid, forgotten.error());
      return pyroceph::tui::make_error_state(
          std::format("cratchit kunde inte ta bort bokföringen ur samlingsmappen: {}", forgotten.error()));
    }
    LOG_BUSINESS("Samlingsmappen {} glömde projektet {} i {}", workspace_.folder.string(), listed_.uuid,
                 listed_.folder.string());
    forgotten_ = true;
    return nullptr; // back to the workspace (back_to)
  }
  State const* back_to() const override { return forgotten_ ? origin_ : nullptr; }
  std::string help_topic() const override { return std::string(help_topic::forget_project); }

private:
  Workspace workspace_;
  ListedProject listed_;
  bool missing_;
  std::string reason_;
  State const* origin_;
  bool forgotten_ = false;
};

} // namespace

std::unique_ptr<State> make_forget_project_menu(Workspace workspace, ListedProject listed, bool missing,
                                                std::string reason, State const* origin) {
  return std::make_unique<ForgetProjectState>(std::move(workspace), std::move(listed), missing, std::move(reason), origin);
}

} // namespace cratchit::ui
