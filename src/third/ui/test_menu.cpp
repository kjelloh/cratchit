#include "ui/test_menu.hpp"

#include "pyroceph/tui/error_state.hpp"

#include <algorithm>
#include <expected>
#include <format>
#include <string>
#include <string_view>

namespace cratchit::ui {

namespace {

using pyroceph::tui::CodePoint;
using pyroceph::tui::InputResult;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

// Each state creates the next one when the user goes there

class OppnaProjektState : public State {
public:
  std::string name() const override { return "Öppna projekt"; }
  View view() const override { return {}; }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
};

class ProjektState : public State {
public:
  std::string name() const override { return "Projekt"; }
  View view() const override { return {}; }
  OptionsList options() const override { return {{U'1', "Öppna"}, {U'2', "Öppna (misslyckas)"}}; }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'1') return std::make_unique<OppnaProjektState>();
    // Shows how a state answers when the state a key maps to cannot be entered
    if (key == U'2') return pyroceph::tui::make_error_state("Kunde inte öppna projektet.");
    return nullptr;
  }
};

// Takes a year of four digits
class BokslutState : public State {
public:
  std::string name() const override { return "Bokslut"; }
  View view() const override { return {}; }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  InputResult on_input(std::u32string_view text) override {
    auto const is_digit = [](char32_t c) { return c >= U'0' && c <= U'9'; };
    if (text.size() != 4 || !std::ranges::all_of(text, is_digit)) {
      return std::unexpected(std::string("Ange ett år med fyra siffror, till exempel 2025."));
    }
    std::string year;
    for (auto const c : text) year.push_back(static_cast<char>(c));
    return std::format("Bokslut för {} noterat.", year);
  }
};

class TestmenyState : public State {
public:
  std::string name() const override { return "Testmeny"; }
  View view() const override { return {}; }
  OptionsList options() const override { return {{U'1', "Projekt"}, {U'2', "Bokslut"}}; }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'1') return std::make_unique<ProjektState>();
    if (key == U'2') return std::make_unique<BokslutState>();
    return nullptr;
  }
};

} // namespace

std::unique_ptr<State> make_test_menu() { return std::make_unique<TestmenyState>(); }

} // namespace cratchit::ui
