#include "ui/reports_menu.hpp"

#include "charset/utf8.hpp"
#include "core/books/books.hpp"
#include "core/books/report.hpp"
#include "core/date.hpp"
#include "core/log.hpp"
#include "core/path_value.hpp"
#include "core/save_document.hpp"
#include "pyroceph/tui/error_state.hpp"
#include "ui/help_topics.hpp"

#include <algorithm>
#include <cstddef>
#include <expected>
#include <format>
#include <string>
#include <system_error>
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

constexpr CodePoint save_key = U'0';

// The company as a report's heading shows it: the project's name and, when
// project.txt has it, the organisation number, e.g. "Test AB (556000-0001)"
std::string company_of(Project const& project) {
  auto const entries = read_path_values(project.file);
  auto const orgnr = entries ? find_value(*entries, "orgnr").value_or("") : std::string();
  return orgnr.empty() ? project.name : std::format("{} ({})", project.name, orgnr);
}

// How a report of the year is saved: as "<project> <what> <year>", e.g.
// "Test AB Balansrapport 2025-26" (the year's "/" becomes "-" in the file name)
SaveAs save_as(Project const& project, FiscalYear const& year, std::string_view what, FindFolder const& downloads) {
  return {project.folder, std::format("{} {} {}", project.name, what, label_of(year)), downloads};
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
  ReportState(std::string name, std::vector<std::string> lines, std::optional<SaveAs> save)
      : name_(std::move(name)), lines_(std::move(lines)), save_(std::move(save)) {}
  std::string name() const override { return name_; }
  // An empty line before and after, for air on the screen (beslut 2026-11-18);
  // under them what saving did
  View view() const override {
    View lines{""};
    lines.insert(lines.end(), lines_.begin(), lines_.end());
    lines.push_back("");
    lines.insert(lines.end(), saved_.begin(), saved_.end());
    return lines;
  }
  OptionsList options() const override {
    if (!save_) return {};
    return {{save_key, "Spara som fil"}};
  }
  // Spara som fil: saved at once, told below the report; the user stays
  bool on_key_in_place(CodePoint key) override {
    if (key != save_key || !save_) return false;
    saved_ = save();
    return true;
  }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::string help_topic() const override { return std::string(help_topic::report); }

private:
  // Saves to the downloads folder, else the fallback folder; what to tell
  View save() const {
    View told;
    auto folder = save_->fallback_folder;
    auto const downloads = save_->downloads ? save_->downloads()
                                            : std::expected<std::filesystem::path, std::string>(
                                                  std::unexpected(std::string("ingen mapp given")));
    if (downloads && fs_is_directory(*downloads)) {
      folder = *downloads;
    } else {
      told.push_back(downloads ? std::format("Mappen Hämtade filer ({}) finns inte, så filen sparades i bokföringens mapp.",
                                             utf8_of(*downloads))
                               : std::format("Hittar inte mappen Hämtade filer ({}), så filen sparades i bokföringens mapp.",
                                             downloads.error()));
    }
    auto const saved = save_document(folder, save_->title, lines_);
    if (!saved) {
      LOG_BUSINESS("Kunde inte spara {} i {}: {}", save_->title, utf8_of(folder), saved.error());
      return {std::format("cratchit kunde inte spara filen: {}", saved.error())};
    }
    LOG_BUSINESS("Sparade {} som {}", save_->title, utf8_of(*saved));
    told.insert(told.begin(), std::format("cratchit sparade filen {}", utf8_of(*saved)));
    return told;
  }
  static bool fs_is_directory(std::filesystem::path const& path) {
    std::error_code ec;
    return std::filesystem::is_directory(path, ec);
  }

  std::string name_;
  View lines_;
  std::optional<SaveAs> save_;
  View saved_; // what saving did, until the user goes on
};

class ReportsState : public State {
public:
  ReportsState(Project project, FiscalYear year, FindFolder downloads)
      : project_(std::move(project)), year_(std::move(year)), downloads_(std::move(downloads)) {}
  std::string name() const override { return "Rapporter"; }
  View view() const override { return {std::format("Rapporter för räkenskapsåret {}.", label_of(year_))}; }
  OptionsList options() const override {
    return {{U'1', "Balansrapport"}, {U'2', "Resultatrapport"}, {U'3', "Verifikationslista"}, {U'4', "Huvudbok"}};
  }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'4') return make_ledger_menu(project_, year_, downloads_);
    if (key != U'1' && key != U'2' && key != U'3') return nullptr;
    auto const year = read_year(project_, year_);
    if (!year) return pyroceph::tui::make_error_state(year.error());
    auto const& [in_books, index] = *year;
    auto const company = company_of(project_);
    auto const report = [&](std::string name, std::vector<std::string> lines) {
      auto save = save_as(project_, year_, name, downloads_);
      return make_report_view(std::move(name), std::move(lines), std::move(save));
    };
    if (key == U'1') return report("Balansrapport", books::balance_report(in_books, index, company));
    if (key == U'2') return report("Resultatrapport", books::result_report(in_books, index, company));
    return report("Verifikationslista", books::voucher_list(in_books, index, company));
  }
  std::string help_topic() const override { return std::string(help_topic::reports); }

private:
  Project project_;
  FiscalYear year_;
  FindFolder downloads_;
};

class LedgerState : public State {
public:
  LedgerState(Project project, FiscalYear year, FindFolder downloads)
      : project_(std::move(project)), year_(std::move(year)), downloads_(std::move(downloads)) {}
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
    if (!typed.empty() && !std::ranges::all_of(typed, [](char c) { return c >= '0' && c <= '9'; })) {
      return std::unexpected(std::string("Skriv kontots nummer med bara siffror, till exempel 1930."));
    }
    auto const year = read_year(project_, year_);
    if (!year) return std::unexpected(year.error());
    auto const& [in_books, index] = *year;
    if (typed.empty()) {
      return Accepted(make_report_view("Alla konton", books::general_ledger(in_books, index, company_of(project_)),
                                       save_as(project_, year_, "Huvudbok alla konton", downloads_)));
    }
    auto ledger = books::account_ledger(in_books, index, company_of(project_), typed);
    if (!ledger) {
      return std::unexpected(
          std::format("Konto {} finns inte i bokföringen och har inga rader i {}.", typed, label_of(year_)));
    }
    return Accepted(make_report_view(std::format("Huvudbok {}", typed), std::move(*ledger),
                                     save_as(project_, year_, std::format("Huvudbok {}", typed), downloads_)));
  }
  std::string help_topic() const override { return std::string(help_topic::ledger_account); }

private:
  Project project_;
  FiscalYear year_;
  FindFolder downloads_;
};

} // namespace

std::expected<std::unique_ptr<State>, std::string> make_voucher_printout(Project const& project, FiscalYear const& year,
                                                                         books::Voucher const& voucher,
                                                                         FindFolder downloads) {
  auto const in_year = read_year(project, year);
  if (!in_year) return std::unexpected(in_year.error());
  auto const id = books::voucher_id(voucher.series, voucher.number);
  return make_report_view("Utskrift", books::voucher_document(in_year->books, in_year->index, voucher, company_of(project)),
                          save_as(project, year, std::format("Verifikation {}", id), std::move(downloads)));
}

std::unique_ptr<State> make_reports_menu(Project project, FiscalYear year, FindFolder downloads) {
  return std::make_unique<ReportsState>(std::move(project), std::move(year), std::move(downloads));
}

std::unique_ptr<State> make_report_view(std::string name, std::vector<std::string> lines, std::optional<SaveAs> save) {
  return std::make_unique<ReportState>(std::move(name), std::move(lines), std::move(save));
}

std::unique_ptr<State> make_ledger_menu(Project project, FiscalYear year, FindFolder downloads) {
  return std::make_unique<LedgerState>(std::move(project), std::move(year), std::move(downloads));
}

} // namespace cratchit::ui
