#include "core/skv/vat_return/cli.hpp"

#include "core/amount.hpp"
#include "core/books/books.hpp"
#include "core/books/import.hpp"
#include "core/date.hpp"
#include "core/sie/check.hpp"
#include "core/sie/document.hpp"
#include "core/sie/file.hpp"
#include "core/sie_summary.hpp"
#include "core/skv/vat_return/checks.hpp"
#include "core/skv/vat_return/clearing.hpp"
#include "core/skv/vat_return/document.hpp"
#include "core/skv/vat_return/eskd.hpp"
#include "core/skv/vat_return/files.hpp"
#include "core/skv/vat_return/manual.hpp"
#include "core/skv/vat_return/report.hpp"
#include "core/skv/vat_return/steps.hpp"

#include <algorithm>
#include <charconv>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <print>
#include <string>
#include <system_error>
#include <vector>

namespace cratchit::skv::vat_return {

namespace {

namespace fs = std::filesystem;

constexpr std::string_view usage = "Usage: cratchit --sie-to-vat <file.se> [<last year's file.se>] [--period <YYYYQn>]";

struct Arguments {
  std::vector<fs::path> files; // one or two SIE files
  std::optional<Period> period;
  bool help = false;
};

// 2026Q3: the calendar quarter
std::optional<Period> period_of(std::string_view text) {
  if (text.size() != 6 || (text[4] != 'Q' && text[4] != 'q')) return std::nullopt;
  int year = 0;
  auto const [end, error] = std::from_chars(text.data(), text.data() + 4, year);
  if (error != std::errc{} || end != text.data() + 4) return std::nullopt;
  if (text[5] < '1' || text[5] > '4') return std::nullopt;
  return quarter_of(std::chrono::year{year}, static_cast<unsigned>(text[5] - '0'));
}

std::expected<Arguments, std::string> parse_arguments(std::span<std::string_view const> arguments) {
  Arguments result;
  for (std::size_t i = 0; i < arguments.size(); ++i) {
    auto const argument = arguments[i];
    if (argument == "-h" || argument == "--help") {
      result.help = true;
    } else if (argument == "--period") {
      if (result.period) return std::unexpected("--period is given twice");
      if (i + 1 == arguments.size()) return std::unexpected("--period needs a quarter, e.g. 2026Q3");
      result.period = period_of(arguments[++i]);
      if (!result.period) return std::unexpected(std::format("--period needs a quarter, e.g. 2026Q3, not '{}'", arguments[i]));
    } else if (argument.starts_with('-')) {
      return std::unexpected(std::format("Unknown option '{}'", argument));
    } else {
      result.files.emplace_back(argument);
    }
  }
  if (result.help) {
    if (arguments.size() != 1) return std::unexpected("--help takes nothing else");
    return result;
  }
  if (result.files.empty()) return std::unexpected("No SIE file given");
  if (result.files.size() > 2) return std::unexpected("At most two SIE files: this year's and last year's");
  return result;
}

// A file read and checked, or the error lines
std::expected<sie::Document, std::vector<std::string>> read_sie(fs::path const& file) {
  auto const lexed = sie::lex_file(file);
  if (!lexed) return std::unexpected(std::vector<std::string>{lexed.error()});
  auto const parsed = sie::parse_document(lexed->lines);
  auto const findings = sie::check_document(parsed.document);
  std::vector<std::string> errors = parsed.errors;
  errors.insert(errors.end(), findings.errors.begin(), findings.errors.end());
  if (!errors.empty()) return std::unexpected(errors);
  if (!parsed.document.year(0)) return std::unexpected(std::vector<std::string>{std::format("{} has no fiscal year 0 (#RAR 0)", file.string())});
  return parsed.document;
}

// The files as books in memory, the older year first; the error lines when an import cannot be made
struct Read {
  books::Books books;
  books::Identity identity;
  sie::Document latest; // the file of the latest year: the year the quarters are listed for
  fs::path latest_file;
};

std::expected<Read, std::vector<std::string>> read_books(std::vector<fs::path> const& files) {
  std::vector<std::pair<sie::Document, fs::path>> documents;
  for (auto const& file : files) {
    auto document = read_sie(file);
    if (!document) {
      auto lines = document.error();
      for (auto& line : lines) line = std::format("{}: {}", file.string(), line);
      return std::unexpected(lines);
    }
    documents.emplace_back(std::move(*document), file);
  }
  std::ranges::sort(documents, {}, [](auto const& d) { return d.first.year(0)->start; });
  Read read;
  for (auto const& [document, file] : documents) {
    auto const plan = books::plan_import(read.books, read.identity, document);
    if (!plan.errors.empty()) {
      auto lines = plan.errors;
      for (auto& line : lines) line = std::format("{}: {}", file.string(), line);
      return std::unexpected(lines);
    }
    read.books = plan.books;
    read.identity = plan.identity;
  }
  read.latest = documents.back().first;
  read.latest_file = documents.back().second;
  return read;
}

// Every quarter ending in a fiscal year of the books, oldest first, so that a charge finds its quarter
std::vector<Period> all_quarters(books::Books const& books) {
  std::vector<Period> periods;
  for (auto const& year : books.years) {
    for (auto const& period : quarters_ending_in(year.start, year.end)) {
      if (std::ranges::find(periods, period) == periods.end()) periods.push_back(period);
    }
  }
  std::ranges::sort(periods, {}, &Period::first);
  return periods;
}

Date first_day(books::Books const& books) { return books.years.front().start; }

std::string utf8_name(fs::path const& path) {
  auto const name = path.filename().u8string();
  return {name.begin(), name.end()};
}

} // namespace

int run_cli(std::span<std::string_view const> arguments_given, std::FILE* out, std::FILE* err, std::string_view version) {
  auto const arguments = parse_arguments(arguments_given);
  if (!arguments) {
    std::println(err, "Error: {}\n{}\nSee cratchit --sie-to-vat --help", arguments.error(), usage);
    return 2;
  }
  if (arguments->help) {
    std::print(out, "{}", manual_text());
    return 0;
  }
  auto const read = read_books(arguments->files);
  if (!read) {
    for (auto const& line : read.error()) std::println(err, "Error: {}", line);
    return 1;
  }
  auto const& books = read->books;
  auto const year = *read->latest.year(0);
  auto const today = today_date();
  auto const periods = all_quarters(books);
  auto const steps = steps_of(books, periods);
  auto const company = std::format("{}, {}", read->identity.name, read->identity.orgnr);

  if (!arguments->period) {
    std::println(out, "VAT quarters ending in {} - {} of {} (today {}):", format_iso_date(year.start),
                 format_iso_date(year.end), company, format_iso_date(today));
    std::println(out, "          {}", steps_heading());
    auto const ending = quarters_ending_in(year.start, year.end);
    for (auto it = ending.rbegin(); it != ending.rend(); ++it) {
      auto const found = std::ranges::find(steps.periods, *it, &PeriodSteps::period);
      if (found == steps.periods.end()) continue;
      std::string note;
      if (it->first > today) {
        continue; // not started
      } else if (it->last >= today) {
        note = "  so far";
      }
      if (it->first < first_day(books)) note += std::format("  (starts before {}: give last year's file too)", format_iso_date(first_day(books)));
      std::println(out, "  {:<8}{}{}", quarter_name(*it), steps_row(*found), note);
    }
    for (auto const& charge : steps.unmatched_charges) {
      if (charge.date < year.start || year.end < charge.date) continue; // as take 3: those dated in the year
      std::println(out, "Charged, not matched to a quarter: {} {}  {}", books::voucher_id(charge.series, charge.number),
                   format_iso_date(charge.date), format_amount(charge.amount));
    }
    // The example: the latest quarter that has ended, else the first of the year
    auto example = ending.front();
    for (auto const& quarter : ending) {
      if (quarter.last < today) example = quarter;
    }
    std::println(out, "Write a quarter's VAT return with --period, e.g. --period {}", quarter_name(example).replace(4, 2, "Q"));
    return 0;
  }

  auto const period = *arguments->period;
  auto const found = std::ranges::find(steps.periods, period, &PeriodSteps::period);
  if (found == steps.periods.end()) {
    std::println(err, "Error: {} does not end in a fiscal year of the files ({} - {})", quarter_name(period),
                 format_iso_date(first_day(books)), format_iso_date(books.years.back().end));
    return 1;
  }
  if (period.first < first_day(books)) {
    std::println(err, "Error: {} starts {}, before the files' first fiscal year ({}): give last year's SIE file too",
                 quarter_name(period), format_iso_date(period.first), format_iso_date(first_day(books)));
    return 1;
  }
  if (period.first > today) {
    std::println(err, "Error: {} has not started (today {})", quarter_name(period), format_iso_date(today));
    return 1;
  }
  bool const running = period.last >= today;
  // The return of the period's vouchers, without its VAT clearing: the one to file, also when it is cleared already
  auto const document = document_of(books, period, Clearings::left_out);
  auto const checks = checks_of(books, period);
  auto const report = report_with_checks(
      books, document, *found, company, running ? std::optional(today) : std::nullopt,
      std::format("Gjord {} av cratchit {} --sie-to-vat från {}", format_iso_date(today), version,
                  utf8_name(read->latest_file)));

  auto const stem = std::format("{}_moms_{}_{}", read->identity.orgnr, format_iso_date(period.first),
                                format_iso_date(period.last));
  auto const folder = fs::current_path() / stem;
  std::error_code error;
  fs::create_directories(folder, error);
  if (error) {
    std::println(err, "Error: cannot create the folder {}: {}", folder.string(), error.message());
    return 1;
  }
  std::println(out, "Folder {}", folder.string());
  if (auto written = write_text(folder / (stem + "_momsrapport.txt"), report); !written) {
    std::println(err, "Error: {}", written.error());
    return 1;
  }
  std::println(out, "  Wrote {}_momsrapport.txt: {}{}", stem,
               checks.empty() ? std::string("no checks to look at") : std::format("{} checks to look at", checks.size()),
               running ? " (so far: the quarter has not ended)" : "");
  if (running) {
    std::println(out, "  Did not write the eSKD file or the clearing voucher: the quarter has not ended");
    return 0;
  }
  auto const voucher = voucher_lines(books, clearing_of(document), *found);
  if (auto written = write_text(folder / (stem + "_verifikation.txt"), voucher);
      !written) {
    std::println(err, "Error: {}", written.error());
    return 1;
  }
  if (found->clearings.empty()) {
    std::println(out, "  Wrote {}_verifikation.txt: the clearing voucher to book (series {})", stem, clearing_series);
  } else {
    std::println(out, "  Wrote {}_verifikation.txt: already cleared by {}", stem, vouchers_text(found->clearings));
  }
  auto const eskd_document = eskd::document_of(document, read->identity.orgnr);
  auto const eskd_file = folder / eskd::file_name(eskd_period(period));
  {
    std::ofstream eskd_out(eskd_file, std::ios::binary);
    eskd_out << eskd::xml_of(eskd_document);
    if (!eskd_out) {
      std::println(err, "Error: cannot write {}", eskd_file.string());
      return 1;
    }
  }
  std::println(out, "  Wrote {}: upload it in SKV's e-service Lämna momsdeklaration (replaced on every run)",
               utf8_name(eskd_file));
  auto const copy = folder / (stem + ".se");
  if (fs::exists(copy, error) && fs::equivalent(copy, read->latest_file, error)) {
    std::println(out, "  Kept {}.se (the same as the input)", stem);
  } else {
    fs::copy_file(read->latest_file, copy, fs::copy_options::overwrite_existing, error);
    if (error) {
      std::println(err, "Error: cannot copy the SIE file to {}: {}", copy.string(), error.message());
      return 1;
    }
    std::println(out, "  Copied the SIE file as {}.se", stem);
  }
  return 0;
}

} // namespace cratchit::skv::vat_return
