#include "ui/reports_menu.hpp"

#include "charset/utf8.hpp"
#include "core/books/books.hpp"
#include "core/books/report.hpp"
#include "core/date.hpp"
#include "core/log.hpp"
#include "core/path_value.hpp"
#include "pyroceph/tui/error_state.hpp"
#include "ui/help_topics.hpp"

#include <algorithm>
#include <cstddef>
#include <expected>
#include <format>
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

// The company as a report's heading shows it: the project's name and, when
// project.txt has it, the organisation number, e.g. "Test AB (556000-0001)"
std::string company_of(Project const& project) {
  auto const entries = read_path_values(project.file);
  auto const orgnr = entries ? find_value(*entries, "orgnr").value_or("") : std::string();
  return orgnr.empty() ? project.name : std::format("{} ({})", project.name, orgnr);
}

// The books and the index of the fiscal year in them, or why not
struct YearInBooks {
  books::Books books;
  std::size_t index = 0;
};
std::expected<YearInBooks, std::string> read_year(Project const& project, FiscalYear const& year) {
  auto books = books::read_books(project.folder);
  if (!books) {
    LOG_BUSINESS("Kunde inte läsa bokföringen i {}: {}", project.folder.string(), books.error());
    return std::unexpected(std::format("cratchit kunde inte läsa bokföringen: {}", books.error()));
  }
  auto const start = parse_iso_date(year.start);
  auto const* found = start ? books->year_starting(*start) : nullptr;
  if (found == nullptr) return std::unexpected(std::format("Räkenskapsåret {} finns inte i bokföringen.", label_of(year)));
  auto const index = static_cast<std::size_t>(found - books->years.data());
  return YearInBooks{std::move(*books), index};
}

class ReportState : public State {
public:
  ReportState(std::string name, std::vector<std::string> lines) : name_(std::move(name)), lines_(std::move(lines)) {}
  std::string name() const override { return name_; }
  // An empty line before and after, for air on the screen (beslut 2026-11-18)
  View view() const override {
    View lines{""};
    lines.insert(lines.end(), lines_.begin(), lines_.end());
    lines.push_back("");
    return lines;
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::string help_topic() const override { return std::string(help_topic::report); }

private:
  std::string name_;
  View lines_;
};

class ReportsState : public State {
public:
  ReportsState(Project project, FiscalYear year) : project_(std::move(project)), year_(std::move(year)) {}
  std::string name() const override { return "Rapporter"; }
  View view() const override { return {std::format("Rapporter för räkenskapsåret {}.", label_of(year_))}; }
  OptionsList options() const override {
    return {{U'1', "Balansrapport"}, {U'2', "Resultatrapport"}, {U'3', "Verifikationslista"}, {U'4', "Huvudbok"}};
  }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'4') return make_ledger_menu(project_, year_);
    if (key != U'1' && key != U'2' && key != U'3') return nullptr;
    auto const year = read_year(project_, year_);
    if (!year) return pyroceph::tui::make_error_state(year.error());
    auto const& [in_books, index] = *year;
    if (key == U'1') return make_report_view("Balansrapport", books::balance_report(in_books, index, company_of(project_)));
    if (key == U'2') return make_report_view("Resultatrapport", books::result_report(in_books, index, company_of(project_)));
    return make_report_view("Verifikationslista", books::voucher_list(in_books, index, company_of(project_)));
  }
  std::string help_topic() const override { return std::string(help_topic::reports); }

private:
  Project project_;
  FiscalYear year_;
};

class LedgerState : public State {
public:
  LedgerState(Project project, FiscalYear year) : project_(std::move(project)), year_(std::move(year)) {}
  std::string name() const override { return "Huvudbok"; }
  View view() const override {
    return {"Skriv kontots nummer och tryck Enter, till exempel 1930. Tryck bara Enter för alla konton."};
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  // A text field, so that Enter on an empty input reaches on_input: all accounts
  bool is_text_field() const override { return true; }
  InputResult on_input(std::u32string_view text) override {
    std::string typed;
    charset::utf8::Encoder encoder;
    for (auto const code_point : text) {
      for (auto const byte : encoder.push(code_point)) typed.push_back(static_cast<char>(byte));
    }
    if (typed.empty()) {
      auto const year = read_year(project_, year_);
      if (!year) return std::unexpected(year.error());
      auto const& [in_books, index] = *year;
      return Accepted(make_report_view("Alla konton", books::general_ledger(in_books, index, company_of(project_))));
    }
    if (!std::ranges::all_of(typed, [](char c) { return c >= '0' && c <= '9'; })) {
      return std::unexpected(std::string("Skriv kontots nummer med bara siffror, till exempel 1930."));
    }
    auto const year = read_year(project_, year_);
    if (!year) return std::unexpected(year.error());
    auto const& [in_books, index] = *year;
    auto ledger = books::account_ledger(in_books, index, company_of(project_), typed);
    if (!ledger) {
      return std::unexpected(
          std::format("Konto {} finns inte i bokföringen och har inga rader i {}.", typed, label_of(year_)));
    }
    return Accepted(make_report_view(std::format("Huvudbok {}", typed), std::move(*ledger)));
  }
  std::string help_topic() const override { return std::string(help_topic::ledger_account); }

private:
  Project project_;
  FiscalYear year_;
};

} // namespace

std::expected<std::vector<std::string>, std::string> voucher_printout(Project const& project, FiscalYear const& year,
                                                                      books::Voucher const& voucher) {
  auto const in_year = read_year(project, year);
  if (!in_year) return std::unexpected(in_year.error());
  return books::voucher_document(in_year->books, in_year->index, voucher, company_of(project));
}

std::unique_ptr<State> make_reports_menu(Project project, FiscalYear year) {
  return std::make_unique<ReportsState>(std::move(project), std::move(year));
}

std::unique_ptr<State> make_report_view(std::string name, std::vector<std::string> lines) {
  return std::make_unique<ReportState>(std::move(name), std::move(lines));
}

std::unique_ptr<State> make_ledger_menu(Project project, FiscalYear year) {
  return std::make_unique<LedgerState>(std::move(project), std::move(year));
}

} // namespace cratchit::ui
