#include "ui/fiscal_year_menu.hpp"

#include "core/sie_summary.hpp"
#include "core/date.hpp"
#include "core/log.hpp"
#include "core/sie_export.hpp"
#include "core/sie_project.hpp"
#include "pyroceph/tui/error_state.hpp"
#include "pyroceph/tui/path_completion.hpp"
#include "ui/help_topics.hpp"
#include "ui/sie_import_menu.hpp"
#include "ui/year_flow_menu.hpp"
#include "ui/year_view.hpp"

#include <format>
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

constexpr char const* export_caption = "Exportera till SIE";

class FiscalYearState;

// What an export wrote, for the user
std::string written_text(Project const& project, Exported const& exported, bool balances) {
  auto text = std::format("cratchit skrev {} i {}: {} verifikationer{}.", exported.file.filename().string(),
                          project.folder.string(), exported.vouchers, balances ? " och balanser" : ", utan balanser");
  if (exported.unmapped > 0) {
    text += std::format(" {} tecken finns inte i SIE-filens teckenkodning och skrevs som <U+…>.", exported.unmapped);
  }
  return text;
}

std::string voucher_line(books::Voucher const& voucher) {
  return std::format("    {:<6}  {}  {:>12}  {}", books::voucher_id(voucher.series, voucher.number), format_iso_date(voucher.event.date),
                     format_amount(voucher.event.amount), voucher.event.heading);
}

// The fiscal year compared with an SIE file, offering to write the difference
class ComparisonState : public State {
public:
  ComparisonState(FiscalYearState& year, Project project, FiscalYear fiscal_year, SieComparison comparison)
      : year_(year), project_(std::move(project)), fiscal_year_(std::move(fiscal_year)),
        comparison_(std::move(comparison)) {}
  std::string name() const override { return comparison_.sie_file.filename().string(); }
  View view() const override {
    View lines;
    lines.push_back(comparison_.generated.empty()
                        ? std::format("Jämfört med {} ({}):", comparison_.sie_file.filename().string(), comparison_.program)
                        : std::format("Jämfört med {} ({}, skapad {}):", comparison_.sie_file.filename().string(),
                                      comparison_.program, comparison_.generated));
    // The difference is only right against the books in the other program as they are now
    if (comparison_.generated.empty()) {
      lines.push_back("Filen saknar datum (#GEN). Är det den senaste exporten från programmet?");
    } else if (comparison_.generated < today_iso()) {
      lines.push_back(std::format("Filen skapades {}. Är det den senaste exporten från programmet?", comparison_.generated));
    }
    lines.push_back(std::format("  I räkenskapsåret, inte i filen ({}):", comparison_.books_only.size()));
    for (auto const& voucher : comparison_.books_only) lines.push_back(voucher_line(voucher));
    lines.push_back(std::format("  I filen, inte i räkenskapsåret ({}):", comparison_.file_only.size()));
    for (auto const& voucher : comparison_.file_only) lines.push_back(voucher_line(voucher));
    if (!comparison_.file_only.empty()) {
      lines.push_back("cratchit skriver ingen fil: SIE-filen har verifikationer som inte finns i räkenskapsåret, eller som ser annorlunda ut "
                        "där. Program som Fortnox kan bara lägga till verifikationer, så de måste stämma först.");
    } else if (comparison_.books_only.empty()) {
      lines.push_back("Alla verifikationer i räkenskapsåret finns redan i SIE-filen. Det finns inget att exportera.");
    }
    return lines;
  }
  OptionsList options() const override {
    if (!can_write()) return {};
    return {{U'1', "Skriv skillnaden till en SIE-fil"}};
  }
  OnKeyResult on_key(CodePoint key) override;
  State const* back_to() const override;
  std::string help_topic() const override { return std::string(help_topic::sie_comparison); }

private:
  bool can_write() const { return !written_ && comparison_.file_only.empty() && !comparison_.books_only.empty(); }

  FiscalYearState& year_; // below on the path, so it outlives this state
  Project project_;
  FiscalYear fiscal_year_;
  SieComparison comparison_;
  bool written_ = false;
};

// Asks for the SIE file to compare the fiscal year with
class DifferenceState : public State {
public:
  DifferenceState(FiscalYearState& year, Project project, FiscalYear fiscal_year)
      : year_(year), project_(std::move(project)), fiscal_year_(std::move(fiscal_year)) {}
  std::string name() const override { return "Skillnad"; }
  View view() const override {
    return {"Skriv sökvägen till en SIE-fil som du just har exporterat från programmet du ska föra över till, och tryck "
            "Enter. Skillnaden blir bara rätt om filen visar hur bokföringen ser ut där nu. Tab fyller i mappar och SIE-filer."};
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::u32string start_input() const override { return pyroceph::tui::path_input_of(project_.folder); }
  Completion complete(std::u32string_view text) const override {
    return pyroceph::tui::complete_path(
        text, true, [](std::filesystem::path const& file) { return is_sie_file_name(file); }, describe_sie_file,
        list_sie_files);
  }
  InputResult on_input(std::u32string_view text) override {
    auto const file = pyroceph::tui::path_of(text);
    auto comparison = compare_with_sie(project_, fiscal_year_, file);
    if (!comparison) {
      LOG_BUSINESS("Kan inte jämföra räkenskapsåret {} med {}: {}", fiscal_year_.start, file.string(), comparison.error());
      return std::unexpected(std::format("cratchit kan inte jämföra med filen: {}", comparison.error()));
    }
    return Accepted(std::make_unique<ComparisonState>(year_, project_, fiscal_year_, std::move(*comparison)));
  }
  std::string help_topic() const override { return std::string(help_topic::sie_difference); }

private:
  FiscalYearState& year_;
  Project project_;
  FiscalYear fiscal_year_;
};

class ExportState : public State {
public:
  ExportState(FiscalYearState& year, Project project, FiscalYear fiscal_year)
      : year_(year), project_(std::move(project)), fiscal_year_(std::move(fiscal_year)) {}
  std::string name() const override { return export_caption; }
  View view() const override { return {}; }
  OptionsList options() const override { return {{U'1', "Allt"}, {U'2', "Skillnad"}}; }
  OnKeyResult on_key(CodePoint key) override;
  State const* back_to() const override;
  std::string help_topic() const override { return std::string(help_topic::export_sie); }

private:
  FiscalYearState& year_;
  Project project_;
  FiscalYear fiscal_year_;
  bool written_ = false;
};

class FiscalYearState : public State {
public:
  FiscalYearState(Project project, FiscalYear year) : project_(std::move(project)), year_(std::move(year)) {}
  std::string name() const override { return label_of(year_); }
  View view() const override {
    auto const zoom = current_view();
    View lines{std::format("Räkenskapsår {} – {}", year_.start, year_.end), zoom.summary};
    lines.insert(lines.end(), report_.begin(), report_.end());
    lines.insert(lines.end(), zoom.rows.begin(), zoom.rows.end());
    return lines;
  }
  OptionsList options() const override {
    return {{U'0', "Årsflöde"}, {U'1', export_caption}, {U'2', "Importera från SIE"}};
  }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'0') {
      report_.clear();
      return make_year_flow_menu(project_, year_);
    }
    if (key == U'1') {
      report_.clear();
      return std::make_unique<ExportState>(*this, project_, year_);
    }
    if (key == U'2') {
      report_.clear();
      return make_sie_import_menu(project_, year_, this, [this](View text, FiscalYear imported) {
        report_ = std::move(text);
        year_ = std::move(imported); // the file's dates, e.g. a first year of another length or a change of fiscal year
      });
    }
    return nullptr;
  }
  std::string help_topic() const override { return std::string(help_topic::fiscal_year); }

  // Shown in the view, e.g. what an export wrote, until the user goes on from here
  void report(std::string text) { report_ = {std::move(text)}; }

private:
  // The current zoom of the year as it is in the books now (for now the whole year)
  YearView current_view() const {
    auto const books = books::read_books(project_.folder);
    auto const start = parse_iso_date(year_.start);
    return year_view(books && start ? books->year_starting(*start) : nullptr);
  }

  Project project_;
  FiscalYear year_;
  View report_; // what was just done, shown until the user goes on from here
};

OnKeyResult ExportState::on_key(CodePoint key) {
  if (key == U'2') return std::make_unique<DifferenceState>(year_, project_, fiscal_year_);
  if (key != U'1') return nullptr;
  auto const exported = export_all(project_, fiscal_year_, today_iso());
  if (!exported) {
    LOG_BUSINESS("Kunde inte exportera räkenskapsåret {} i {}: {}", fiscal_year_.start, project_.folder.string(), exported.error());
    return pyroceph::tui::make_error_state(std::format("cratchit kunde inte exportera räkenskapsåret: {}", exported.error()));
  }
  LOG_BUSINESS("Exporterade räkenskapsåret {} – {} i projektet {} ({}) till {} ({} verifikationer)", fiscal_year_.start,
               fiscal_year_.end, project_.name, project_.uuid, exported->file.string(), exported->vouchers);
  year_.report(written_text(project_, *exported, true));
  written_ = true;
  return nullptr; // back to the fiscal year (back_to)
}

State const* ExportState::back_to() const { return written_ ? &year_ : nullptr; }

OnKeyResult ComparisonState::on_key(CodePoint key) {
  if (key != U'1' || !can_write()) return nullptr;
  auto const exported = export_difference(project_, fiscal_year_, comparison_, today_iso());
  if (!exported) {
    LOG_BUSINESS("Kunde inte exportera skillnaden mot {}: {}", comparison_.sie_file.string(), exported.error());
    return pyroceph::tui::make_error_state(std::format("cratchit kunde inte exportera skillnaden: {}", exported.error()));
  }
  LOG_BUSINESS("Exporterade skillnaden mellan räkenskapsåret {} – {} i projektet {} ({}) och {} till {} ({} verifikationer)",
               fiscal_year_.start, fiscal_year_.end, project_.name, project_.uuid, comparison_.sie_file.string(),
               exported->file.string(), exported->vouchers);
  year_.report(written_text(project_, *exported, false));
  written_ = true;
  return nullptr; // back to the fiscal year (back_to)
}

State const* ComparisonState::back_to() const { return written_ ? &year_ : nullptr; }

} // namespace

std::unique_ptr<State> make_fiscal_year_menu(Project project, FiscalYear year) {
  return std::make_unique<FiscalYearState>(std::move(project), std::move(year));
}

} // namespace cratchit::ui
