#include "core/ar/cli.hpp"

#include "core/ar/arguments.hpp"
#include "core/closing.hpp"
#include "core/ar/disposition.hpp"
#include "core/ar/equity.hpp"
#include "core/ar/layout.hpp"
#include "core/ar/manual.hpp"
#include "core/ar/notes.hpp"
#include "core/ar/output.hpp"
#include "core/ar/overview.hpp"
#include "core/ar/requirements.hpp"
#include "core/ar/statement.hpp"
#include "core/ar/text.hpp"
#include "core/amount.hpp"
#include "core/date.hpp"
#include "core/sie/check.hpp"
#include "core/sie/document.hpp"
#include "core/sie/file.hpp"

#include <ctime>
#include <filesystem>
#include <format>
#include <optional>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// From cratchit_ar's main.cpp (session 5b35b2b9): the same arguments, output,
// files and exit codes, writing to out and err instead of stdout and stderr.
namespace cratchit::ar {

namespace {

// The local date and time now, e.g. 2026-11-02 18:04
std::string now_text() {
  auto const now = std::time(nullptr);
  std::tm local{};
#ifdef _WIN32
  localtime_s(&local, &now);
#else
  localtime_r(&now, &local);
#endif
  char text[32];
  std::strftime(text, sizeof(text), "%Y-%m-%d %H:%M", &local);
  return text;
}

} // namespace

int run_cli(std::span<std::string_view const> arguments_given, std::FILE* out, std::FILE* err, std::string_view version) {
  auto const arguments = parse_arguments(std::vector<std::string_view>(arguments_given.begin(), arguments_given.end()));
  if (!arguments) {
    std::println(err, "Error: {}\n{}\nSee cratchit --sie-to-ar --help", arguments.error(), usage);
    return 2;
  }
  if (arguments->help) {
    std::print(out, "{}", manual_text());
    return 0;
  }
  bool const trace = arguments->trace;
  auto const bas_year = arguments->bas_year.value_or(default_bas_year);
  auto const mapping = layout_for(bas_year);
  if (!mapping) {
    std::println(err, "Error: no mapping for BAS {}; the oldest is for BAS {}\n{}\nSee cratchit --sie-to-ar --help", bas_year,
                 built_in_layouts().front().year, usage);
    return 2;
  }
  auto const& file = arguments->file;
  // Errors and notes are collected: --trace prints them, a normal run writes them to the report
  std::vector<std::string> errors;
  std::vector<std::string> notes_found;
  auto const lexed = cratchit::sie::lex_file(file);
  if (!lexed) {
    std::println(err, "Error: {}", lexed.error());
    return 1;
  }
  if (!lexed->alert.empty()) {
    notes_found.push_back(lexed->alert);
  }
  auto const parsed = cratchit::sie::parse_document(lexed->lines);
  auto const findings = cratchit::sie::check_document(parsed.document);
  // An SIE file that cannot be read is not worth a folder: its errors go to the terminal only
  if (!parsed.errors.empty() || !findings.errors.empty()) {
    for (auto const& error : parsed.errors) {
      std::println(err, "Error: {}", error);
    }
    for (auto const& error : findings.errors) {
      std::println(err, "Error: {}", error);
    }
    return 1;
  }
  for (auto const& note : findings.notes) {
    notes_found.push_back(note);
  }
  auto const mapping_text =
      std::format("{} ({}; books following BAS {}{})", mapping->file, bas_years_of(*mapping), bas_year,
                  arguments->bas_year ? "" : ", the default; see --bas-year");
  auto const layout = parse_layout(mapping->text);
  if (!layout) {
    std::println(err, "Error: {}", layout.error());
    return 1;
  }
  // Books not yet closed: show the årsredovisning as if the year's result were booked, and say so
  auto const unbooked = unbooked_result(parsed.document);
  auto const document = unbooked ? with_closing_entry(parsed.document, *unbooked) : parsed.document;
  if (auto const year = document.year(0); unbooked && year) {
    notes_found.push_back(std::format(
        "Closing: the books for {} - {} are not closed: the year's result {} is not booked to 2099 (nothing on 8999). "
        "The årsredovisning is a preview that shows it as booked (see --trace).",
        cratchit::format_iso_date(year->start), cratchit::format_iso_date(year->end), cratchit::format_amount(-*unbooked)));
  }
  // The year of the file and the year before, when the file has it
  std::vector<cratchit::sie::FiscalYear> years;
  std::vector<Balances> balances;
  for (int index = 0; index >= -1; --index) {
    if (auto const year = document.year(index)) {
      years.push_back(*year);
      balances.push_back(year_balances(document, index));
    }
  }
  auto const statement = build_statement(*layout, balances);
  errors.insert(errors.end(), statement.errors.begin(), statement.errors.end());
  for (auto const& finding : check_statement(statement)) {
    errors.push_back(finding);
  }
  // Closing entries BAS asks for that the books lack; the årsredovisning shows the books as they are
  for (std::size_t index = 0; index < years.size(); ++index) {
    for (auto const& note : check_closing(balances[index], years[index].end)) {
      notes_found.push_back("Closing: " + note);
    }
  }
  auto const equity = equity_changes(*layout, document);
  errors.insert(errors.end(), equity.errors.begin(), equity.errors.end());
  notes_found.insert(notes_found.end(), equity.notes.begin(), equity.notes.end());
  auto const notes = build_notes(statement, document);
  errors.insert(errors.end(), notes.errors.begin(), notes.errors.end());
  auto const overview = build_overview(statement);
  auto const disposition = build_disposition(statement);
  auto const check = check_requirements(statement, overview, notes, unbooked);
  int const exit_code = errors.empty() ? 0 : 1;
  for (auto const& error : errors) {
    std::println(err, "Error: {}", error);
  }

  if (trace) {
    std::println(err, "Note: mapping {}", mapping_text);
    for (auto const& note : notes_found) {
      std::println(err, "Note: {}", note);
    }
    std::println(out, "Mapping: {}", mapping_text);
    std::print(out, "{}", format_trace(overview, disposition, equity, statement, notes, document, years));
    return exit_code;
  }

  // The årsredovisning, in the order of BFNAR 2016:10 p. 3.2: förvaltningsberättelse, resultaträkning,
  // balansräkning, noter; none when there are errors
  std::optional<std::string> ar;
  if (errors.empty()) {
    ar = std::format("{}\n{}\n{}\n{}\n{}\n{}\n{}\n{}", format_front(document, years, check),
                     format_business(check), format_overview(overview, years, check),
                     format_equity_changes(equity), format_disposition(disposition, check),
                     format_statements(statement, years, note_references(notes)),
                     format_notes(notes, years, check),
                     format_signatures(document, years, check));
  }
  auto const paths = output_paths(std::filesystem::current_path(), document.orgnr, years.front());
  auto const items = std::format("{} {} TBD", check.missing.size(), check.missing.size() == 1 ? "item" : "items");
  auto const status = !errors.empty() ? std::format("{} {}: the årsredovisning was not written",
                                                    errors.size(), errors.size() == 1 ? "error" : "errors")
                      : unbooked     ? std::format("PRELIMINÄR: the books are not closed; {}", items)
                      : check.missing.empty() ? std::string("complete")
                                              : std::format("not complete; {}", items);
  std::string sie_details;
  if (!document.program.empty()) {
    sie_details = std::format("#PROGRAM {} {}", document.program, document.program_version);
  }
  if (document.generated) {
    sie_details += std::format("{}#GEN {}", sie_details.empty() ? "" : ", ", cratchit::format_iso_date(*document.generated));
  }
  ReportHeader const header{paths.ar.filename().string(),
                                          now_text(),
                                          std::format("cratchit {} --sie-to-ar", version),
                                          file.string(),
                                          paths.sie.filename().string(),
                                          sie_details,
                                          mapping_text,
                                          status};
  auto const report = format_report(header, errors, notes_found,
                                                  format_check(check, mapping_text));
  auto const written = write_outputs(paths, ar, report, file);
  if (!written) {
    std::println(err, "Error: {}", written.error());
    return 1;
  }
  std::println(out, "Folder {}", paths.folder.string());
  if (written->ar) {
    std::println(out, "  Wrote {}: {}", paths.ar.filename().string(), status);
    std::println(out, "    (replaced on every run: copy it before editing)");
  } else {
    std::println(out, "  Did not write {}: {} (an earlier one is left as it was)", paths.ar.filename().string(), status);
  }
  std::println(out, "  Wrote {}: {} {}, the K2 check", paths.report.filename().string(), notes_found.size(),
               notes_found.size() == 1 ? "note" : "notes");
  if (written->sie_copied) {
    std::println(out, "  Copied the SIE file as {}", paths.sie.filename().string());
  } else {
    std::println(out, "  Kept {}{}", paths.sie.filename().string(),
                 written->ar ? " (the same as the input)" : " (it matches the earlier årsredovisning)");
  }
  return exit_code;
}

} // namespace cratchit::ar
