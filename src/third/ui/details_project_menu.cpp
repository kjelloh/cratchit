#include "ui/details_project_menu.hpp"

#include "charset/utf8.hpp"
#include "core/details_project.hpp"
#include "core/log.hpp"
#include "core/project.hpp"
#include "pyroceph/tui/error_state.hpp"
#include "pyroceph/tui/prompt_loop.hpp"
#include "ui/help_topics.hpp"
#include "ui/workspace_menu.hpp"

#include <chrono>
#include <cstdint>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <utility>

namespace cratchit::ui {

namespace {

using pyroceph::tui::Accepted;
using pyroceph::tui::CodePoint;
using pyroceph::tui::InputResult;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

std::string utf8_of(std::u32string_view text) {
  charset::utf8::Encoder encoder;
  std::string result;
  for (auto const code_point : text) {
    for (auto const byte : encoder.push(code_point)) result.push_back(static_cast<char>(byte));
  }
  return result;
}

std::u32string u32_of(std::string_view utf8) {
  charset::utf8::Decoder decoder;
  std::u32string result;
  for (auto const c : utf8) {
    if (auto const code_point = decoder.push(static_cast<std::uint8_t>(c))) result.push_back(*code_point);
  }
  return result;
}

// The first fiscal year must be the current one or at most 9 before it (beslut.md); first_month names it in the error
std::expected<void, std::string> within_handled_years(FiscalYear const& year, std::string_view first_month) {
  if (years_before_current(year, today_iso()) <= max_older_fiscal_years) return {};
  return std::unexpected(std::format("cratchit hanterar bara det aktuella räkenskapsåret och {} äldre. {} är för tidigt.",
                                     max_older_fiscal_years, first_month));
}

// The details of the form, each empty (or nothing) until the user gives it
struct Details {
  std::string name;
  std::filesystem::path folder; // <workspace>/<name>, or the working folder when made here
  std::string orgnr;            // NNNNNN-NNNN
  std::optional<std::chrono::year_month> first_month;
  int months = 12; // the first fiscal year's length

  bool complete() const { return !name.empty() && !orgnr.empty() && first_month.has_value(); }
  FiscalYear year() const { return first_fiscal_year(*first_month, months); }
};

enum class Field { name, orgnr, first_month, months };

class DetailsFormState;

// One field of the form, typed in a text field that starts with the field's
// current value. Enter takes a value the field accepts into the form and goes
// back to it; - goes back while the text is untouched, leaving the field as it was.
class FieldState : public State {
public:
  FieldState(DetailsFormState& form, Field field) : form_(form), field_(field) {}
  std::string name() const override;
  View view() const override;
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  bool is_text_field() const override { return true; }
  std::u32string start_input() const override;
  InputResult on_input(std::u32string_view text) override;
  State const* back_to() const override;
  std::string help_topic() const override;

private:
  DetailsFormState& form_; // below this field on the path, so it outlives it
  Field field_;
  bool taken_ = false;
};

class DetailsFormState : public State {
public:
  DetailsFormState(Workspace workspace, State const* origin, std::optional<std::filesystem::path> here)
      : workspace_(std::move(workspace)), origin_(origin), here_(here.has_value()) {
    if (here) details_.folder = *here;
  }
  std::string name() const override { return "Från uppgifter"; }
  bool here() const { return here_; }
  View view() const override {
    if (created_) return {std::format("cratchit har skapat bokföringen i {}.", details_.folder.string())};
    if (!details_.complete()) {
      View view{"Välj en uppgift och fyll i den. När namn, organisationsnummer och räkenskapsår är ifyllda skapar Enter bokföringen."};
      if (here_) view.push_back(std::format("Bokföringen hamnar i {}.", details_.folder.string()));
      return view;
    }
    return {std::format("Tryck Enter för att skapa bokföringen i {}.", details_.folder.string()),
            "Bokföringen får ingen kontoplan än."};
  }
  OptionsList options() const override {
    if (created_) return {};
    auto const or_ask = [](std::string const& value, std::string const& ask) { return value.empty() ? ask : value; };
    auto const year = details_.first_month ? std::format("{} – {}", details_.year().start, details_.year().end) : std::string();
    return {{U'1', "Namn: " + or_ask(details_.name, "<ange organisationens namn>")},
            {U'2', "Organisationsnummer: " + or_ask(details_.orgnr, "<ange organisationens organisationsnummer>")},
            {U'3', "Räkenskapsår: " + or_ask(year, "<ange räkenskapsårets första månad>")},
            {U'4', std::format("Första årets längd: {} månader", details_.months)}};
  }
  OnKeyResult on_key(CodePoint key) override {
    if (created_) return nullptr;
    if (key == U'1') return std::make_unique<FieldState>(*this, Field::name);
    if (key == U'2') return std::make_unique<FieldState>(*this, Field::orgnr);
    if (key == U'3') return std::make_unique<FieldState>(*this, Field::first_month);
    if (key == U'4') return std::make_unique<FieldState>(*this, Field::months);
    if (pyroceph::tui::is_enter(key) && details_.complete()) return create();
    return nullptr;
  }
  State const* back_to() const override { return created_ ? origin_ : nullptr; }
  // Made here (in the working folder): on to the workspace, which lists it
  std::unique_ptr<State> after_back() override {
    if (!created_ || !here_) return nullptr;
    return make_workspace_after_created(workspace_, details_.name);
  }
  std::string help_topic() const override { return std::string(help_topic::from_details); }

  // The value a field shows, as typed
  std::string value_of(Field field) const {
    switch (field) {
    case Field::name: return details_.name;
    case Field::orgnr: return details_.orgnr;
    case Field::first_month:
      return details_.first_month ? std::format("{:04}-{:02}", static_cast<int>(details_.first_month->year()),
                                                static_cast<unsigned>(details_.first_month->month()))
                                  : std::string();
    case Field::months: return std::to_string(details_.months);
    }
    return {};
  }

  // Takes text into a field, or says why it cannot
  std::expected<void, std::string> take(Field field, std::string const& text) {
    switch (field) {
    case Field::name: {
      if (here_) {
        auto const first = text.find_first_not_of(' ');
        if (first == std::string::npos) return std::unexpected(std::string("Skriv organisationens namn."));
        details_.name = text.substr(first, text.find_last_not_of(' ') - first + 1);
        return {};
      }
      auto folder = new_project_folder(workspace_, text);
      if (!folder) return std::unexpected(folder.error());
      details_.folder = *folder;
      details_.name = folder->filename().string();
      return {};
    }
    case Field::orgnr: {
      auto orgnr = parse_orgnr(text);
      if (!orgnr) return std::unexpected(orgnr.error());
      details_.orgnr = *orgnr;
      return {};
    }
    case Field::first_month: {
      auto month = parse_first_month(text);
      if (!month) return std::unexpected(month.error());
      if (auto const handled = within_handled_years(first_fiscal_year(*month, details_.months), text); !handled) {
        return std::unexpected(handled.error());
      }
      details_.first_month = *month;
      return {};
    }
    case Field::months: {
      auto months = parse_first_year_length(text);
      if (!months) return std::unexpected(months.error());
      if (details_.first_month) {
        auto const handled = within_handled_years(first_fiscal_year(*details_.first_month, *months),
                                                  std::format("{:04}-{:02}", static_cast<int>(details_.first_month->year()),
                                                              static_cast<unsigned>(details_.first_month->month())));
        if (!handled) return std::unexpected(handled.error());
      }
      details_.months = *months;
      return {};
    }
    }
    return {};
  }

private:
  OnKeyResult create() {
    auto const year = details_.year();
    auto project = here_ ? create_project_here(details_.folder, details_.name, details_.orgnr, year, today_iso())
                         : create_project_from_details(details_.folder, details_.name, details_.orgnr, year, today_iso());
    if (!project) {
      LOG_BUSINESS("Kunde inte skapa bokföringen {}: {}", details_.name, project.error());
      return pyroceph::tui::make_error_state(std::format("cratchit kunde inte skapa bokföringen: {}", project.error()));
    }
    created_ = true;
    LOG_BUSINESS("Skapade projektet {} ({}) i {} med räkenskapsåret {} – {}", project->name, details_.orgnr,
                 project->folder.string(), year.start, year.end);
    if (auto const remembered = remember_project(workspace_, *project); !remembered) {
      LOG_BUSINESS("Samlingsmappen kunde inte komma ihåg projektet: {}", remembered.error());
      return pyroceph::tui::make_error_state(
          std::format("cratchit skapade bokföringen men kunde inte skriva in den i samlingsmappen: {}", remembered.error()));
    }
    return nullptr; // back to where the user started (back_to)
  }

  Workspace workspace_;
  State const* origin_;
  Details details_;
  bool here_; // made in the working folder itself, not in a new folder in the workspace
  bool created_ = false;
};

std::string FieldState::name() const {
  switch (field_) {
  case Field::name: return "Namn";
  case Field::orgnr: return "Organisationsnummer";
  case Field::first_month: return "Räkenskapsår";
  case Field::months: return "Första årets längd";
  }
  return {};
}

View FieldState::view() const {
  switch (field_) {
  case Field::name:
    if (form_.here()) return {"Skriv organisationens namn och tryck Enter."};
    return {"Skriv organisationens namn och tryck Enter. Bokföringen får en mapp med det namnet i samlingsmappen."};
  case Field::orgnr: return {"Skriv organisationsnumret och tryck Enter, till exempel 556677-8899."};
  case Field::first_month: return {"Skriv räkenskapsårets första månad och tryck Enter, till exempel 2026-05."};
  case Field::months:
    return {"Skriv hur många månader det första räkenskapsåret är och tryck Enter, 1 till 18. Oftast 12, men ett nytt "
            "företags första år kan vara kortare eller längre."};
  }
  return {};
}

std::u32string FieldState::start_input() const { return u32_of(form_.value_of(field_)); }

InputResult FieldState::on_input(std::u32string_view text) {
  if (auto const taken = form_.take(field_, utf8_of(text)); !taken) return std::unexpected(taken.error());
  taken_ = true;
  return Accepted(); // back to the form (back_to)
}

State const* FieldState::back_to() const { return taken_ ? &form_ : nullptr; }

std::string FieldState::help_topic() const {
  switch (field_) {
  case Field::name: return std::string(form_.here() ? help_topic::here_name : help_topic::details_name);
  case Field::orgnr: return std::string(help_topic::details_orgnr);
  case Field::first_month: return std::string(help_topic::details_year);
  case Field::months: return std::string(help_topic::details_months);
  }
  return {};
}

} // namespace

std::unique_ptr<State> make_project_from_details(Workspace workspace, State const* origin) {
  return std::make_unique<DetailsFormState>(std::move(workspace), origin, std::nullopt);
}

std::unique_ptr<State> make_project_here(Workspace workspace, std::filesystem::path folder, State const* origin) {
  return std::make_unique<DetailsFormState>(std::move(workspace), origin, std::move(folder));
}

} // namespace cratchit::ui
