#include "ui/sie_import_menu.hpp"

#include "core/sie_summary.hpp"
#include "core/date.hpp"
#include "core/fiscal_year.hpp"
#include "core/log.hpp"
#include "core/sie_import.hpp"
#include "core/sie_project.hpp"
#include "pyroceph/tui/error_state.hpp"
#include "pyroceph/tui/path_completion.hpp"
#include "ui/help_topics.hpp"

#include <algorithm>
#include <format>
#include <optional>
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

using Report = std::function<void(View, FiscalYear)>;

// "1 ny verifikation", "2 nya verifikationer"
std::string new_vouchers_text(std::size_t count) {
  return count == 1 ? "1 ny verifikation" : std::format("{} nya verifikationer", count);
}

// One series compared, e.g. "A  bokföringen t.o.m. A 59, filen t.o.m. A 63, nya: A 60–63"
std::string series_text(SeriesComparison const& s, std::size_t width) {
  auto const name = s.series + std::string(width > s.series.size() ? width - s.series.size() : 0, ' ');
  std::string text;
  if (s.books_last.empty()) {
    text = std::format("bara i filen, t.o.m. {}{}", s.series, s.file_last);
  } else if (s.file_last.empty()) {
    text = std::format("bara i bokföringen, t.o.m. {}{}", s.series, s.books_last);
  } else if (s.books_last == s.file_last && s.new_numbers.empty()) {
    text = std::format("lika, t.o.m. {}{}", s.series, s.file_last);
  } else {
    text = std::format("bokföringen t.o.m. {}{}, filen t.o.m. {}{}", s.series, s.books_last, s.series, s.file_last);
  }
  if (!s.new_numbers.empty()) text += std::format(", nya: {}{}", s.series, s.new_numbers);
  return "  " + name + "  " + text;
}

class ConfirmImportState : public State {
public:
  ConfirmImportState(Project project, SieImportPlan plan, FiscalYear year, State const* origin, State const* chooser,
                     Report report)
      : project_(std::move(project)), plan_(std::move(plan)), year_(std::move(year)), origin_(origin),
        chooser_(chooser), report_(std::move(report)), summary_(summarize_sie_file(plan_.sie_file)) {}
  std::string name() const override { return "Bekräfta"; }
  View view() const override {
    if (imported_) return {"cratchit har importerat SIE-filen."};
    auto const& import = plan_.import;
    View lines;
    lines.push_back(std::format("SIE-fil: {}", plan_.sie_file.string()));
    if (!plan_.alert.empty()) lines.push_back(plan_.alert);
    auto const row = [&lines](std::string_view label, std::string const& value) {
      lines.push_back(std::format("  {:<16}{}", std::string(label) + ":", value));
    };
    auto const status = summary_ ? status_of(*summary_, today_date()) : std::nullopt;
    auto year = std::format("{} – {}", format_iso_date(import.year_start), format_iso_date(import.year_end));
    if (status) year += ", " + status_text(SieStatus{status->status, false});
    if (plan_.expected) year += std::format(" (cratchit räknade med {} – {})", plan_.expected->start, plan_.expected->end);
    row("Räkenskapsår", year);
    if (plan_.expected) {
      row("Ansvar", "cratchit kontrollerar inte Skatteverkets godkännande av en omläggning. Du ansvarar för att "
                    "räkenskapsåret är korrekt.");
    }
    if (!plan_.generated.empty() || !plan_.program.empty()) {
      auto created = plan_.generated;
      if (!plan_.program.empty()) created += created.empty() ? plan_.program : std::format(" ({})", plan_.program);
      row("Skapad", created);
    }
    if (summary_) {
      auto vouchers = std::to_string(summary_->vouchers);
      if (summary_->last_date) vouchers += " t.o.m. " + format_iso_date(*summary_->last_date);
      row("Verifikationer", vouchers);
    }
    if (status && status->warning) row("Varning", "Ett räkenskapsår före förra året ska vara fullbordat. Kontrollera SIE-filen.");
    if (status && status->status == YearStatus::mismatch) {
      row("Varning", std::format("Ett resultat är bokfört på 8999, men balansräkningen balanserar inte (skillnad {}).",
                                 format_amount(summary_->balance)));
    }

    lines.push_back(plan_.new_year ? "Jämfört med bokföringen (räkenskapsåret är nytt):"
                                   : std::format("Jämfört med bokföringen ({}):", label_of(year_)));
    std::size_t width = 0;
    for (auto const& s : plan_.series) width = std::max(width, s.series.size());
    for (auto const& s : plan_.series) {
      lines.push_back(series_text(s, width));
      for (auto const& v : plan_.in_the_way) {
        if (v.series != s.series) continue;
        lines.push_back(std::format("      {}{}  {}  {}  {}  {}", v.series, v.number, v.date, v.heading,
                                    format_amount(v.amount), v.why));
      }
    }
    if (!plan_.changed.empty()) {
      lines.push_back("  Ändrade (strukna och tillagda rader):");
      for (std::size_t i = 0; i < shown(); ++i) lines.push_back("    " + change_text(plan_.changed[i]));
      if (shown() < plan_.changed.size()) {
        lines.push_back(std::format("    … och {} till. Tryck 2 för fler.", plan_.changed.size() - shown()));
      }
    }
    if (plan_.possible()) {
      lines.push_back(std::format("  Konton: {} nya, {} med ändrat namn eller ändrade uppgifter", import.accounts_added,
                                  import.accounts_changed));
      if (import.identity_changed) lines.push_back(std::format("  Bokföringen får organisationsnummer {}.", import.identity.orgnr));
      auto what = new_vouchers_text(import.vouchers_added);
      if (!plan_.changed.empty()) what += std::format(", {} {}", plan_.changed.size(), plan_.changed.size() == 1 ? "ändrad" : "ändrade");
      lines.push_back(std::format("Importen går att göra: {}.", what));
      lines.push_back("Importera SIE-filen?");
    } else if (plan_.replaceable) {
      std::string why;
      for (auto const& blocker : plan_.blockers) why += " " + blocker;
      lines.push_back("Importen går inte att göra." + why);
      lines.push_back("Ersätt året med SIE-filens innehåll?");
    } else {
      lines.push_back("Importen går inte att göra:");
      for (auto const& blocker : plan_.blockers) lines.push_back("  " + blocker);
    }
    return lines;
  }
  OptionsList options() const override {
    if (imported_) return {};
    OptionsList result;
    if (plan_.possible() || plan_.replaceable) {
      result = {{U'J', "Ja"}, {U'N', "Nej"}};
    } else {
      result = {{U'N', "Tillbaka"}};
    }
    if (shown() < plan_.changed.size()) result.push_back({more_key, "Mer…"});
    return result;
  }
  // Mer… shows one page more of the changed vouchers
  bool on_key_in_place(CodePoint key) override {
    if (imported_ || key != more_key || shown() >= plan_.changed.size()) return false;
    pages_ += 1;
    return true;
  }
  OnKeyResult on_key(CodePoint key) override {
    if (imported_) return nullptr;
    if (key == U'N') { // keys match exactly (beslut.md)
      declined_ = true;
      return nullptr;
    }
    if (key != U'J') return nullptr;
    if (plan_.replaceable) return make_replace_state();
    if (!plan_.possible()) return nullptr;
    auto const imported = import_sie(project_, plan_, today_date());
    if (!imported) {
      LOG_BUSINESS("Kunde inte importera {} i {}: {}", plan_.sie_file.string(), project_.folder.string(), imported.error());
      return pyroceph::tui::make_error_state(std::format("cratchit kunde inte importera SIE-filen: {}", imported.error()));
    }
    imported_ = true;
    LOG_BUSINESS("Importerade {} (sha256 {}) i projektet {} ({}): räkenskapsåret {} – {}, {} nya verifikationer, {} nya "
                 "konton; filen ligger kvar",
                 plan_.sie_file.string(), imported->entry.sha256, project_.name, project_.uuid,
                 format_iso_date(plan_.import.year_start), format_iso_date(plan_.import.year_end),
                 plan_.import.vouchers_added, plan_.import.accounts_added);
    for (auto const& changed : plan_.changed) LOG_BUSINESS("SIE-import ändrade verifikationen {}", change_text(changed));
    for (auto const& note : plan_.import.notes) LOG_BUSINESS("SIE-import: {}", note);
    if (report_) {
      FiscalYear const year{format_iso_date(plan_.import.year_start), format_iso_date(plan_.import.year_end)};
      report_({std::format("cratchit importerade {}: {} och {} {} i räkenskapsåret {} – {}.",
                           plan_.sie_file.filename().string(), new_vouchers_text(plan_.import.vouchers_added),
                           plan_.changed.size(), plan_.changed.size() == 1 ? "ändrad" : "ändrade", year.start, year.end)},
              year);
    }
    return nullptr; // back to the fiscal year (back_to)
  }
  // Imported: back to the fiscal year. N: back to choose another file.
  State const* back_to() const override {
    if (imported_) return origin_;
    return declined_ ? chooser_ : nullptr;
  }
  std::string help_topic() const override { return std::string(help_topic::confirm_import); }

private:
  // The changed vouchers are listed a page at a time; Mer… shows one page more
  static constexpr std::size_t page = 10;
  static constexpr CodePoint more_key = U'2';
  std::size_t shown() const { return std::min(plan_.changed.size(), page * pages_); }
  std::unique_ptr<State> make_replace_state() const;

  Project project_;
  SieImportPlan plan_;
  FiscalYear year_;     // the fiscal year the user stands in
  State const* origin_; // the fiscal year, gone back to once imported
  State const* chooser_; // where the file was chosen, gone back to on N
  Report report_;
  std::expected<SieSummary, std::string> summary_; // the file's status and last date, read once
  std::size_t pages_ = 1;
  bool imported_ = false;
  bool declined_ = false;
};

// The vouchers that are gone, one per line, e.g. "D 27  2026-04-12  Loopia Faktura  311.25  finns inte i filen"
std::vector<std::string> gone_lines(std::vector<VoucherInTheWay> const& gone, std::string_view indent) {
  std::vector<std::string> lines;
  for (auto const& v : gone) {
    lines.push_back(std::format("{}{}{}  {}  {}  {}  {}", indent, v.series, v.number, v.date, v.heading,
                                format_amount(v.amount), v.why));
  }
  return lines;
}

// "Ersätt": what replacing the year with the file takes away and brings in, and J to burn the bridges
class ReplaceState : public State {
public:
  ReplaceState(Project project, SieImportPlan plan, FiscalYear year, State const* origin, State const* chooser, Report report)
      : project_(std::move(project)), plan_(std::move(plan)), year_(std::move(year)), origin_(origin), chooser_(chooser),
        report_(std::move(report)) {}
  std::string name() const override { return "Ersätt"; }
  View view() const override {
    View lines{std::format("Räkenskapsåret {} blir exakt som SIE-filen.", label_of(year_))};
    lines.push_back("Det här försvinner ur bokföringen:");
    auto const gone = gone_lines(plan_.in_the_way, "  ");
    lines.insert(lines.end(), gone.begin(), gone.end());
    std::vector<std::string> coming;
    for (auto const& s : plan_.series) {
      if (!s.new_numbers.empty()) coming.push_back(std::format("{}{}", s.series, s.new_numbers));
    }
    if (!coming.empty()) {
      std::string text;
      for (auto const& c : coming) text += (text.empty() ? "" : ", ") + c;
      lines.push_back("Det här kommer in från filen:");
      lines.push_back("  " + text);
    }
    if (plan_.year_after) lines.push_back("Ingående balanser i följande räkenskapsår räknas om.");
    if (plan_.year_closed) lines.push_back("Varning: räkenskapsåret är stängt. Det ersätts ändå och förblir stängt.");
    lines.push_back("Ersätt räkenskapsåret med SIE-filen?");
    return lines;
  }
  OptionsList options() const override {
    if (replaced_) return {};
    return {{U'J', "Ja"}, {U'N', "Nej"}};
  }
  OnKeyResult on_key(CodePoint key) override {
    if (replaced_) return nullptr;
    if (key == U'N') { // keys match exactly (beslut.md)
      declined_ = true;
      return nullptr;
    }
    if (key != U'J') return nullptr;
    auto const replaced = replace_year_with_sie(project_, plan_, today_date());
    if (!replaced) {
      LOG_BUSINESS("Kunde inte ersätta {} med {}: {}", label_of(year_), plan_.sie_file.string(), replaced.error());
      return pyroceph::tui::make_error_state(std::format("cratchit kunde inte ersätta räkenskapsåret: {}", replaced.error()));
    }
    replaced_ = true;
    if (report_) {
      View text{std::format("cratchit ersatte räkenskapsåret med {}: {} verifikationer, alla från filen.",
                            plan_.sie_file.filename().string(), replaced->vouchers)};
      if (!replaced->removed.empty()) {
        text.push_back("Det här försvann. Bokför det igen från underlag eller kontoutdrag om det behövs:");
        auto const gone = gone_lines(replaced->removed, "  ");
        text.insert(text.end(), gone.begin(), gone.end());
      }
      report_(std::move(text), FiscalYear{format_iso_date(plan_.import.year_start), format_iso_date(plan_.import.year_end)});
    }
    return nullptr; // back to the fiscal year (back_to)
  }
  // Replaced: back to the fiscal year. N: back to choose another file.
  State const* back_to() const override {
    if (replaced_) return origin_;
    return declined_ ? chooser_ : nullptr;
  }
  std::string help_topic() const override { return std::string(help_topic::replace_sie); }

private:
  Project project_;
  SieImportPlan plan_;
  FiscalYear year_;
  State const* origin_;
  State const* chooser_;
  Report report_;
  bool replaced_ = false;
  bool declined_ = false;
};

std::unique_ptr<State> ConfirmImportState::make_replace_state() const {
  return std::make_unique<ReplaceState>(project_, plan_, year_, origin_, chooser_, report_);
}

class ImportSieState : public State {
public:
  ImportSieState(Project project, FiscalYear year, State const* origin, Report report)
      : project_(std::move(project)), year_(std::move(year)), origin_(origin), report_(std::move(report)) {}
  std::string name() const override { return "Importera från SIE"; }
  View view() const override { return {"Skriv sökvägen till SIE-filen och tryck Enter. Tab fyller i mappar och SIE-filer."}; }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  // The folder of the file chosen last, so that going back from "Bekräfta" offers its neighbours
  std::u32string start_input() const override {
    return pyroceph::tui::path_input_of(chosen_folder_.value_or(project_.folder));
  }
  Completion complete(std::u32string_view text) const override {
    return pyroceph::tui::complete_path(
        text, true, [](std::filesystem::path const& file) { return is_sie_file_name(file); }, describe_sie_file,
        list_sie_files);
  }
  InputResult on_input(std::u32string_view text) override {
    auto const file = pyroceph::tui::path_of(text);
    chosen_folder_ = file.parent_path();
    auto plan = plan_sie_import(project_, file, year_);
    if (!plan) {
      LOG_BUSINESS("Kan inte importera {} i {}: {}", file.string(), project_.folder.string(), plan.error());
      return std::unexpected(std::format("cratchit kan inte importera filen: {}", plan.error()));
    }
    return Accepted(std::make_unique<ConfirmImportState>(project_, std::move(*plan), year_, origin_, this, report_));
  }
  std::string help_topic() const override { return std::string(help_topic::import_sie); }

private:
  Project project_;
  FiscalYear year_;
  State const* origin_;
  Report report_;
  std::optional<std::filesystem::path> chosen_folder_;
};

} // namespace

std::unique_ptr<State> make_sie_import_menu(Project project, FiscalYear year, State const* origin, Report report) {
  return std::make_unique<ImportSieState>(std::move(project), std::move(year), origin, std::move(report));
}

} // namespace cratchit::ui
