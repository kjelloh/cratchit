#pragma once

#include "core/date.hpp"
#include "core/fiscal_year.hpp"
#include "core/project.hpp"
#include "pyroceph/tui/state.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace cratchit::ui {

// What the command lines for a fiscal year are made of, read from the project
struct CommandHintFacts {
  std::filesystem::path folder;                  // the project folder, where the commands run
  std::string orgnr;                             // empty when the project has none
  std::optional<std::filesystem::path> sie;      // the latest sie_<year>_allt*.se exported, if any
  std::string output_folder;                     // <orgnr>_<start>_<end>, the folder the commands write
  std::optional<std::string> last_return_folder; // <orgnr>_<start>_<day before the year> holding a BLANKETTER.SRU
  std::optional<Date> start;                     // the fiscal year
  std::optional<Date> end;
  std::optional<std::filesystem::path> last_sie; // the latest export of the year before, when it is in the books
  bool last_year_in_books = false;               // the books have the year before
};

// Reads the facts for year of project (see latest_export_file and the output
// folders of --sie-to-ar and --sie-to-ink2). Never fails: what cannot be read is left out.
CommandHintFacts command_hint_facts(Project const& project, FiscalYear const& year);

// The view of Årsredovisning: how to make the årsredovisning with
// cratchit --sie-to-ar, with the commands for this project and year, as the
// shell of the system cratchit runs on takes them (cd and less, or cd /d and
// more on Windows). Steps: export the year (1 Exportera till SIE > 1 Allt),
// read the help, run in the project folder (and --trace).
std::vector<std::string> annual_report_hint(CommandHintFacts const& facts);

// The view of Deklaration till SKV: likewise for cratchit --sie-to-ink2, with
// last year's return folder when it is there, else --previous-deficit.
std::vector<std::string> tax_return_hint(CommandHintFacts const& facts);

// Årsredovisning and Deklaration till SKV in Årsflöde: the hints above, read
// when the user comes there. No options, no input.
std::unique_ptr<pyroceph::tui::State> make_annual_report_menu(Project project, FiscalYear year);
std::unique_ptr<pyroceph::tui::State> make_tax_return_menu(Project project, FiscalYear year);

} // namespace cratchit::ui
