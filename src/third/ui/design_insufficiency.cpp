#include "ui/design_insufficiency.hpp"

#include "core/log.hpp"
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

class DesignInsufficiencyState : public State {
public:
  DesignInsufficiencyState(std::string name, std::string description)
      : name_(std::move(name)), description_(std::move(description)) {}
  std::string name() const override { return name_; }
  View view() const override { return {description_}; }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::string help_topic() const override { return std::string(help_topic::design_insufficiency); }

private:
  std::string name_;
  std::string description_;
};

} // namespace

std::unique_ptr<State> make_design_insufficiency_state(std::string_view option, std::string_view what,
                                                       std::source_location where) {
  log::write(log::Kind::DesignInsufficiency, where, std::format("{}: {}", option, what));
  return std::make_unique<DesignInsufficiencyState>(std::format("Inte än ({})", option),
                                                    std::format("Utvecklaren av cratchit har inte infört {} ännu.", what));
}

} // namespace cratchit::ui
