#include "ui/sie_project_menu.hpp"

#include "core/sie_summary.hpp"
#include "core/fiscal_year.hpp"
#include "core/log.hpp"
#include "core/project.hpp"
#include "core/sie_import.hpp"
#include "core/sie_project.hpp"
#include "pyroceph/tui/error_state.hpp"
#include "pyroceph/tui/path_completion.hpp"
#include "ui/help_topics.hpp"
#include "ui/workspace_menu.hpp"

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

std::string date_text(Date const& date) { return format_iso_date(date); }

class ConfirmSieProjectState : public State {
public:
  ConfirmSieProjectState(Workspace workspace, SieProjectPlan plan, State const* origin, State const* from_sie, bool here)
      : workspace_(std::move(workspace)), plan_(std::move(plan)), origin_(origin), from_sie_(from_sie), here_(here),
        summary_(summarize_sie_file(plan_.sie_file)) {}
  std::string name() const override { return "Bekräfta"; }
  View view() const override {
    if (created_) return {std::format("cratchit har skapat bokföringen {}.", plan_.name)};
    View lines;
    lines.push_back(std::format("SIE-fil: {}", plan_.sie_file.string()));
    if (!plan_.alert.empty()) lines.push_back(plan_.alert);
    auto const row = [&lines](std::string_view label, std::string const& value) {
      lines.push_back(std::format("  {:<16}{}", std::string(label) + ":", value));
    };
    row("Mapp", plan_.folder.string());
    row("Företag", plan_.orgnr.empty() ? plan_.name : std::format("{} ({})", plan_.name, plan_.orgnr));
    auto const status = summary_ ? status_of(*summary_, today_date()) : std::nullopt;
    auto year = std::format("{} – {}", date_text(plan_.import.year_start), date_text(plan_.import.year_end));
    if (status) year += ", " + status_text(SieStatus{status->status, false});
    row("Räkenskapsår", year);
    if (status && status->warning) {
      row("Varning", "Ett räkenskapsår före förra året ska vara fullbordat. Kontrollera SIE-filen.");
    }
    FiscalYear const file_year{date_text(plan_.import.year_start), date_text(plan_.import.year_end)};
    if (years_before_current(file_year, today_iso()) > max_older_fiscal_years) {
      row("Varning", std::format("cratchit visar bara det aktuella räkenskapsåret och {} äldre. {} kommer inte att synas.",
                                 max_older_fiscal_years, label_of(file_year)));
    }
    if (status && status->status == YearStatus::mismatch) {
      row("Varning", std::format("Ett resultat är bokfört på 8999, men balansräkningen balanserar inte (skillnad {}).",
                                 format_amount(summary_->balance)));
    }
    if (plan_.generated || !plan_.program.empty()) {
      std::string created = plan_.generated ? date_text(*plan_.generated) : std::string();
      if (!plan_.program.empty()) created += created.empty() ? plan_.program : std::format(" ({})", plan_.program);
      row("Skapad", created);
    }
    auto vouchers = std::to_string(plan_.import.vouchers_added);
    if (summary_ && summary_->last_date) vouchers += " t.o.m. " + date_text(*summary_->last_date);
    if (summary_ && !summary_->series.empty()) {
      std::string series;
      for (auto const& [name, number] : summary_->series) series += (series.empty() ? "" : " · ") + name + number;
      vouchers += "  " + series;
    }
    row("Verifikationer", vouchers);
    row("Konton", std::to_string(plan_.import.books.accounts.size()));
    lines.push_back("Skapa bokföringen från SIE-filen?");
    return lines;
  }
  OptionsList options() const override {
    if (created_) return {};
    return {{U'J', "Ja"}, {U'N', "Nej"}};
  }
  // Made here (in the working folder): on to the workspace, which lists it
  std::unique_ptr<State> after_back() override {
    if (!created_ || !here_) return nullptr;
    return make_workspace_after_created(workspace_, plan_.name);
  }
  // N goes back to "Från SIE", to choose another file
  State const* back_to() const override {
    if (created_) return origin_;
    return declined_ ? from_sie_ : nullptr;
  }
  OnKeyResult on_key(CodePoint key) override {
    if (created_) return nullptr;
    if (key == U'N') { // keys match exactly (beslut.md): J and N, not j and n
      declined_ = true;
      return nullptr;
    }
    if (key != U'J') return nullptr;
    auto project = create_project_from_sie(plan_, today_iso());
    if (!project) {
      LOG_BUSINESS("Kunde inte skapa bokföringen från {}: {}", plan_.sie_file.string(), project.error());
      return pyroceph::tui::make_error_state(std::format("cratchit kunde inte skapa bokföringen: {}", project.error()));
    }
    created_ = true;
    LOG_BUSINESS("Skapade projektet {} i {} från {} ({} verifikationer)", project->name, project->folder.string(),
                 plan_.sie_file.string(), plan_.import.vouchers_added);
    for (auto const& note : plan_.import.notes) LOG_BUSINESS("SIE-import: {}", note);
    if (auto const remembered = remember_project(workspace_, *project); !remembered) {
      LOG_BUSINESS("Samlingsmappen kunde inte komma ihåg projektet: {}", remembered.error());
      return pyroceph::tui::make_error_state(
          std::format("cratchit skapade bokföringen men kunde inte skriva in den i samlingsmappen: {}", remembered.error()));
    }
    return nullptr; // back to where the user started (back_to)
  }
  std::string help_topic() const override { return std::string(help_topic::confirm_sie_project); }

private:
  Workspace workspace_;
  SieProjectPlan plan_;
  State const* origin_;   // where the user started, gone back to once created
  State const* from_sie_; // where the file was chosen, gone back to on N
  bool here_;             // chosen under "Ny bokföring här"
  std::expected<SieSummary, std::string> summary_; // the file's last date and series, read once
  bool created_ = false;
  bool declined_ = false;
};

class FromSieState : public State {
public:
  FromSieState(Workspace workspace, std::filesystem::path start_folder, State const* origin,
               std::optional<std::filesystem::path> here)
      : workspace_(std::move(workspace)), start_folder_(std::move(start_folder)), origin_(origin), here_(std::move(here)) {}
  std::string name() const override { return "Från SIE"; }
  View view() const override {
    return {"Skriv sökvägen till SIE-filen och tryck Enter. Tab fyller i mappar och SIE-filer."};
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  // The folder of the file chosen last, so that going back from "Bekräfta" offers its neighbours
  std::u32string start_input() const override { return pyroceph::tui::path_input_of(chosen_folder_.value_or(start_folder_)); }
  Completion complete(std::u32string_view text) const override {
    return pyroceph::tui::complete_path(
        text, true, [](std::filesystem::path const& file) { return is_sie_file_name(file); }, describe_sie_file,
        list_sie_files);
  }
  InputResult on_input(std::u32string_view text) override {
    auto const file = pyroceph::tui::path_of(text);
    chosen_folder_ = file.parent_path();
    auto const home = user_home();
    auto plan = plan_project_from_sie(file, workspace_.folder, home ? std::optional(*home) : std::nullopt, here_);
    if (!plan) {
      LOG_BUSINESS("Kan inte skapa bokföring från {}: {}", file.string(), plan.error());
      return std::unexpected(std::format("cratchit kan inte skapa bokföringen: {}", plan.error()));
    }
    return Accepted(std::make_unique<ConfirmSieProjectState>(workspace_, std::move(*plan), origin_, this, here_.has_value()));
  }
  std::string help_topic() const override { return std::string(help_topic::from_sie); }

private:
  Workspace workspace_;
  std::filesystem::path start_folder_;
  State const* origin_;
  std::optional<std::filesystem::path> here_; // the working folder, under "Ny bokföring här"
  std::optional<std::filesystem::path> chosen_folder_;
};

} // namespace

std::unique_ptr<State> make_project_from_sie(Workspace workspace, std::filesystem::path start_folder,
                                             State const* origin, std::optional<std::filesystem::path> here) {
  return std::make_unique<FromSieState>(std::move(workspace), std::move(start_folder), origin, std::move(here));
}

} // namespace cratchit::ui
