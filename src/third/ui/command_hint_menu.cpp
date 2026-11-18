#include "ui/command_hint_menu.hpp"

#include "core/ar/output.hpp"
#include "core/books/export.hpp"
#include "core/date.hpp"
#include "core/ink2/previous.hpp"
#include "core/books/books.hpp"
#include "core/sie/document.hpp"
#include "core/sie_summary.hpp"
#include "core/skv/vat_return/document.hpp"
#include "core/skv/vat_return/report.hpp"
#include "ui/help_topics.hpp"

#include <algorithm>
#include <chrono>
#include <format>
#include <string_view>
#include <system_error>
#include <utility>

namespace cratchit::ui {

namespace {

namespace fs = std::filesystem;

using pyroceph::tui::CodePoint;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

#ifdef _WIN32
constexpr std::string_view change_folder = "cd /d";
constexpr std::string_view pager = "more";
#else
constexpr std::string_view change_folder = "cd";
constexpr std::string_view pager = "less";
#endif

std::string utf8_of(fs::path const& path) {
  auto const text = path.u8string();
  return {text.begin(), text.end()};
}

// A word for the shell: in double quotes when it has a space
std::string quoted(std::string const& word) {
  return word.find(' ') == std::string::npos ? word : std::format("\"{}\"", word);
}

// The SIE file for the commands, or a placeholder and how to make it
std::string sie_word(CommandHintFacts const& facts) {
  return facts.sie ? quoted(utf8_of(facts.sie->filename())) : std::string("<SIE-fil>");
}

std::string export_line(CommandHintFacts const& facts) {
  if (!facts.sie) {
    return "   Det finns ingen export av räkenskapsåret i bokföringens mapp än. Exportera den först.";
  }
  return std::format("   Senast exporterad: {}. Exportera igen om du har bokfört något sedan dess.",
                     utf8_of(facts.sie->filename()));
}

std::string output_folder_text(CommandHintFacts const& facts) {
  return facts.output_folder.empty() ? std::string("<orgnr>_<start>_<slut>") : facts.output_folder;
}

class HintState : public State {
public:
  HintState(std::string name, std::string topic, Project project, FiscalYear year,
            std::vector<std::string> (*hint)(CommandHintFacts const&))
      : name_(std::move(name)), topic_(std::move(topic)), project_(std::move(project)), year_(std::move(year)),
        hint_(hint) {}
  std::string name() const override { return name_; }
  // Read each time it is shown, so that an export made meanwhile shows
  View view() const override { return hint_(command_hint_facts(project_, year_)); }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::string help_topic() const override { return topic_; }

private:
  std::string name_;
  std::string topic_;
  Project project_;
  FiscalYear year_;
  std::vector<std::string> (*hint_)(CommandHintFacts const&);
};

// The latest sie_<year>_allt*.se of year in folder (an export never overwrites, so there may be several)
std::optional<fs::path> latest_export(fs::path const& folder, books::FiscalYear const& year) {
  std::optional<fs::path> latest;
  std::error_code ec;
  for (int index = 1; index < 1000; ++index) {
    auto const file = folder / books::export_file_name(year, books::ExportKind::all, index);
    if (!fs::exists(file, ec)) break;
    latest = file;
  }
  return latest;
}

} // namespace

CommandHintFacts command_hint_facts(Project const& project, FiscalYear const& year) {
  CommandHintFacts facts;
  facts.folder = project.folder;
  facts.orgnr = describe_project(ListedProject{project.uuid, project.folder}).orgnr;
  auto const start = parse_iso_date(year.start);
  auto const end = parse_iso_date(year.end);
  if (!start || !end) return facts;
  facts.start = start;
  facts.end = end;
  facts.sie = latest_export(project.folder, books::FiscalYear{*start, *end});
  std::error_code ec;
  // The year before, as the books have it: for a VAT quarter that starts in it
  auto const day_before_year = Date{std::chrono::sys_days{*start} - std::chrono::days{1}};
  if (auto const books = books::read_books(project.folder)) {
    for (auto const& last : books->years) {
      if (last.end != day_before_year) continue;
      facts.last_year_in_books = true;
      facts.last_sie = latest_export(project.folder, books::FiscalYear{last.start, last.end});
    }
  }
  if (facts.orgnr.empty()) return facts;
  facts.output_folder = utf8_of(ar::output_paths(project.folder, facts.orgnr, sie::FiscalYear{0, *start, *end}).folder.filename());
  // Last year's return: a folder <orgnr>_<start>_<the day before this year> (as
  // --sie-to-ink2 names it) holding a BLANKETTER.SRU, whether or not that year is in the books
  auto const day_before = Date{std::chrono::sys_days{*start} - std::chrono::days{1}};
  auto const prefix = facts.output_folder.substr(0, facts.output_folder.size() - std::string("YYYY-MM-DD_YYYY-MM-DD").size());
  auto const suffix = "_" + format_iso_date(day_before);
  for (fs::directory_iterator it(project.folder, ec), stop; !ec && it != stop; it.increment(ec)) {
    auto const name = utf8_of(it->path().filename());
    if (!it->is_directory(ec) || !name.starts_with(prefix) || !name.ends_with(suffix)) continue;
    auto const sru = ink2::sru_file_in(it->path());
    if (sru && sru->filename() != it->path().filename()) facts.last_return_folder = name;
  }
  return facts;
}

std::vector<std::string> annual_report_hint(CommandHintFacts const& facts) {
  auto const sie = sie_word(facts);
  return {
      "Årsredovisningen gör cratchit från kommandoraden, av en SIE-fil för räkenskapsåret.",
      "",
      "1. Exportera räkenskapsåret till en SIE-fil: 1 Exportera till SIE > 1 Allt i räkenskapsåret.",
      export_line(facts),
      "",
      "2. Läs hjälpen, bland annat om --bas-year:",
      std::format("     cratchit --sie-to-ar --help | {}", pager),
      "",
      "3. Avsluta cratchit (qq) och kör i bokföringens mapp:",
      std::format("     {} {}", change_folder, quoted(utf8_of(facts.folder))),
      std::format("     cratchit --sie-to-ar {}", sie),
      std::format("   cratchit skapar mappen {} med årsredovisningen, en rapport och en kopia av SIE-filen.",
                  output_folder_text(facts)),
      "",
      "   Se varifrån varje belopp kommer:",
      std::format("     cratchit --sie-to-ar --trace {} | {}", sie, pager),
  };
}

std::vector<std::string> tax_return_hint(CommandHintFacts const& facts) {
  auto const sie = sie_word(facts);
  std::vector<std::string> lines{
      "Inkomstdeklaration 2 (INK2) gör cratchit från kommandoraden, av en SIE-fil för räkenskapsåret och",
      "förra årets deklaration (för underskottet).",
      "",
      "1. Exportera räkenskapsåret till en SIE-fil: 1 Exportera till SIE > 1 Allt i räkenskapsåret.",
      export_line(facts),
      "",
      "2. Läs hjälpen:",
      std::format("     cratchit --sie-to-ink2 --help | {}", pager),
      "",
      "3. Avsluta cratchit (qq) och kör i bokföringens mapp:",
      std::format("     {} {}", change_folder, quoted(utf8_of(facts.folder))),
  };
  if (facts.last_return_folder) {
    lines.push_back(std::format("     cratchit --sie-to-ink2 {} {}", sie, *facts.last_return_folder));
    lines.push_back(std::format("   {} är mappen med förra årets BLANKETTER.SRU, som --sie-to-ink2 skapade.",
                                *facts.last_return_folder));
    lines.push_back("   Har du förra årets deklaration någon annanstans, ange dess BLANKETTER.SRU eller mapp i stället.");
  } else {
    lines.push_back(std::format("     cratchit --sie-to-ink2 {} <förra årets BLANKETTER.SRU eller dess mapp>", sie));
    lines.push_back("   Har du inte förra årets deklaration, ange förra årets underskott i kronor:");
    lines.push_back(std::format("     cratchit --sie-to-ink2 {} --previous-deficit 0", sie));
  }
  lines.push_back(std::format("   cratchit skapar mappen {} med deklarationen, en rapport, en kopia av SIE-filen",
                              output_folder_text(facts)));
  lines.push_back("   och, när inget saknas, SRU-filerna till Skatteverkets filöverföring.");
  lines.push_back("");
  lines.push_back("   Se hur varje belopp räknas fram:");
  auto const previous = facts.last_return_folder ? *facts.last_return_folder : std::string("--previous-deficit 0");
  lines.push_back(std::format("     cratchit --sie-to-ink2 --trace {} {} | {}", sie, previous, pager));
  return lines;
}

std::unique_ptr<State> make_annual_report_menu(Project project, FiscalYear year) {
  return std::make_unique<HintState>("Årsredovisning", std::string(help_topic::annual_report), std::move(project),
                                     std::move(year), annual_report_hint);
}

std::unique_ptr<State> make_tax_return_menu(Project project, FiscalYear year) {
  return std::make_unique<HintState>("Deklaration till SKV", std::string(help_topic::tax_return), std::move(project),
                                     std::move(year), tax_return_hint);
}

} // namespace cratchit::ui
