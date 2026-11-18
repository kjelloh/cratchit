#include "ui/year_flow_menu.hpp"

#include "ui/booked_menu.hpp"
#include "ui/command_hint_menu.hpp"
#include "ui/draft_menu.hpp"
#include "ui/design_insufficiency.hpp"
#include "ui/help_topics.hpp"
#include "ui/vat_return_menu.hpp"

#include <string>
#include <utility>

namespace cratchit::ui {

namespace {

using pyroceph::tui::CodePoint;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

class ClosingState : public State {
public:
  std::string name() const override { return "Bokslut"; }
  View view() const override { return {"Bokslutet avslutar räkenskapsåret."}; }
  OptionsList options() const override { return {{U'0', "Avstämning"}}; }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'0') return make_design_insufficiency_state("Avstämning", "avstämning mot kontoutdrag");
    return nullptr;
  }
  std::string help_topic() const override { return std::string(help_topic::closing); }
};

class YearFlowState : public State {
public:
  YearFlowState(Project project, FiscalYear year) : project_(std::move(project)), year_(std::move(year)) {}
  std::string name() const override { return "Årsflöde"; }
  View view() const override { return {"Arbetet med räkenskapsåret, i den ordning det brukar göras."}; }
  OptionsList options() const override {
    return {{U'0', "Utkast"},         {U'1', "Bokfört"},     {U'2', "Momsrapport"},
            {U'3', "Bokslut"},        {U'4', "Årsredovisning"}, {U'5', "Årsstämma"},
            {U'6', "Rapport till Bolagsverket"}, {U'7', "Deklaration till SKV"}};
  }
  OnKeyResult on_key(CodePoint key) override {
    switch (key) {
    case U'0': return make_draft_menu(project_, year_);
    case U'1': return make_booked_menu(project_, year_);
    case U'2': return make_vat_return_menu(project_, year_);
    case U'3': return make_closing_menu();
    case U'4': return make_annual_report_menu(project_, year_);
    case U'5': return make_design_insufficiency_state("Årsstämma", "årsstämman");
    case U'6': return make_design_insufficiency_state("Rapport till Bolagsverket", "rapporten till Bolagsverket");
    case U'7': return make_tax_return_menu(project_, year_);
    default: return nullptr;
    }
  }
  std::string help_topic() const override { return std::string(help_topic::year_flow); }

private:
  Project project_;
  FiscalYear year_;
};

} // namespace

std::unique_ptr<State> make_year_flow_menu(Project project, FiscalYear year) {
  return std::make_unique<YearFlowState>(std::move(project), std::move(year));
}

std::unique_ptr<State> make_closing_menu() { return std::make_unique<ClosingState>(); }

} // namespace cratchit::ui
