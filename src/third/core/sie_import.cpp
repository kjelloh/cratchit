#include "core/sie_import.hpp"

#include "core/inventory.hpp"

#include "core/date.hpp"
#include "core/log.hpp"
#include "core/path_value.hpp"
#include "core/sie/document.hpp"
#include "core/sie/file.hpp"

#include <algorithm>
#include <format>
#include <map>
#include <charconv>
#include <system_error>

namespace cratchit {

namespace fs = std::filesystem;

namespace {

constexpr std::size_t shown_errors = 3;

std::string errors_text(std::vector<std::string> const& errors) {
  std::string text = std::format("SIE-filen har {} fel:", errors.size());
  for (std::size_t i = 0; i < std::min(errors.size(), shown_errors); ++i) text += " " + errors[i] + ".";
  if (errors.size() > shown_errors) text += " ...";
  return text;
}

// The vouchers of the year starting on start that the planned books hold otherwise than the books
// before. Error text when one has another date or text: only struck and added rows are taken.
std::expected<std::vector<ChangedVoucher>, std::string> changed_vouchers(books::Books const& before,
                                                                         books::Books const& after, Date start) {
  std::vector<ChangedVoucher> result;
  auto const* old_year = before.year_starting(start);
  auto const* new_year = after.year_starting(start);
  if (old_year == nullptr || new_year == nullptr) return result;
  for (auto const& old_voucher : old_year->vouchers) {
    auto const found = std::ranges::find_if(new_year->vouchers, [&](books::Voucher const& v) {
      return v.series == old_voucher.series && v.number == old_voucher.number;
    });
    if (found == new_year->vouchers.end() || *found == old_voucher) continue;
    if (found->event.date != old_voucher.event.date || found->event.heading != old_voucher.event.heading) {
      return std::unexpected(std::format("Verifikationen {}{} har ett annat datum eller en annan text i filen. cratchit tar "
                                         "bara emot strukna och tillagda rader i befintliga verifikationer",
                                         old_voucher.series, old_voucher.number));
    }
    ChangedVoucher changed{old_voucher.series, old_voucher.number, format_iso_date(old_voucher.event.date),
                           old_voucher.event.heading};
    auto const removed = [](books::Posting const& p) { return p.kind == books::Posting::Kind::removed; };
    auto const struck_before = static_cast<std::size_t>(std::ranges::count_if(old_voucher.postings, removed));
    auto const struck_after = static_cast<std::size_t>(std::ranges::count_if(found->postings, removed));
    changed.struck = struck_after > struck_before ? struck_after - struck_before : 0;
    changed.added = found->postings.size() > old_voucher.postings.size() ? found->postings.size() - old_voucher.postings.size() : 0;
    result.push_back(std::move(changed));
  }
  return result;
}

// True when the file's year may come instead of year, which is not in the books: it adjoins them (starts the
// day after the latest, or ends the day before the oldest, as year does) and is 1 to 18 months long
bool adjoins(books::Books const& books, FiscalYear const& year, FiscalYear const& file_year) {
  if (books.years.empty()) return false;
  auto const start = parse_iso_date(year.start);
  if (!start || books.year_starting(*start) != nullptr) return false; // in the books: the same dates only
  auto const months = months_of(file_year);
  if (months < 1 || months > 18) return false;
  FiscalYear const oldest{format_iso_date(books.years.front().start), format_iso_date(books.years.front().end)};
  FiscalYear const latest{format_iso_date(books.years.back().start), format_iso_date(books.years.back().end)};
  bool const first_after = year.start == year_after(latest).start;
  bool const before_oldest = year.end == year_before(oldest).end;
  return (first_after && file_year.start == year.start) || (before_oldest && file_year.end == year.end);
}

} // namespace

std::string change_text(ChangedVoucher const& changed) {
  auto const rows = [](std::size_t count, std::string_view one, std::string_view many) {
    return std::format("{} {}", count, count == 1 ? one : many);
  };
  std::string what;
  if (changed.struck > 0) what = rows(changed.struck, "rad struken", "rader strukna");
  if (changed.added > 0) what += (what.empty() ? "" : ", ") + rows(changed.added, "rad tillagd", "rader tillagda");
  if (what.empty()) what = "ändrad";
  return std::format("{}{}  {}  {}: {}", changed.series, changed.number, changed.date, changed.heading, what);
}

namespace {

std::optional<long long> number_of(std::string const& text) {
  long long value = 0;
  auto const [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
  if (ec != std::errc() || end != text.data() + text.size()) return std::nullopt;
  return value;
}

// Numbers as ranges, e.g. {60, 61, 62, 63, 65} -> "60–63, 65"
std::string ranges_text(std::vector<long long> numbers) {
  std::ranges::sort(numbers);
  std::string text;
  for (std::size_t i = 0; i < numbers.size();) {
    auto j = i;
    while (j + 1 < numbers.size() && numbers[j + 1] == numbers[j] + 1) ++j;
    if (!text.empty()) text += ", ";
    text += j == i ? std::to_string(numbers[i]) : std::format("{}–{}", numbers[i], numbers[j]);
    i = j + 1;
  }
  return text;
}

std::string highest(std::vector<std::string> const& numbers) {
  std::optional<long long> best;
  for (auto const& n : numbers) {
    if (auto const v = number_of(n); v && (!best || *v > *best)) best = v;
  }
  return best ? std::to_string(*best) : (numbers.empty() ? std::string() : numbers.back());
}

// The series of the year in the books (may be none) and in the file, and the gaps in the file's series
void compare_series(books::FiscalYear const* year, sie::Document const& document, SieImportPlan& plan) {
  std::map<std::string, std::vector<std::string>> in_books;
  std::map<std::string, std::vector<std::string>> in_file;
  if (year != nullptr) {
    for (auto const& v : year->vouchers) in_books[v.series].push_back(v.number);
  }
  for (auto const& v : document.vouchers) in_file[v.series].push_back(v.number);
  std::map<std::string, SeriesComparison> series;
  for (auto const& [name, numbers] : in_books) series[name].books_last = highest(numbers);
  for (auto const& [name, numbers] : in_file) {
    auto& compared = series[name];
    compared.file_last = highest(numbers);
    auto const& books_numbers = in_books[name];
    std::vector<long long> added;
    std::vector<long long> all;
    for (auto const& n : numbers) {
      auto const v = number_of(n);
      if (!v) continue;
      all.push_back(*v);
      if (std::ranges::find(books_numbers, n) == books_numbers.end()) added.push_back(*v);
    }
    compared.new_numbers = ranges_text(added);
    // The file must join the books without gaps: its numbers in a series follow one another
    std::ranges::sort(all);
    std::vector<long long> missing;
    for (std::size_t i = 1; i < all.size(); ++i) {
      for (auto n = all[i - 1] + 1; n < all[i]; ++n) missing.push_back(n);
    }
    if (!missing.empty()) {
      plan.blockers.push_back(std::format("Serie {} i filen har en lucka: {}{} saknas.", name, name, ranges_text(missing)));
    }
  }
  for (auto& [name, compared] : series) {
    compared.series = name;
    plan.series.push_back(std::move(compared));
  }
}

// The books' vouchers that the file would not leave as they are
void find_in_the_way(books::FiscalYear const* year, books::ImportPlan const& import, SieImportPlan& plan) {
  if (year == nullptr) return;
  auto const named = [](std::vector<std::string> const& names, books::Voucher const& v) {
    return std::ranges::find(names, std::format("{} {}", v.series, v.number)) != names.end();
  };
  for (auto const& v : year->vouchers) {
    std::string why;
    if (named(import.renamed, v)) why = "finns i filen under ett annat nummer";
    else if (named(import.replaced, v)) why = "har annat innehåll i filen";
    else if (named(import.dropped, v)) why = "finns inte i filen (en rättelse och det den rättar)";
    else if (named(import.demoted, v)) why = "finns inte i filen";
    if (why.empty()) continue;
    plan.in_the_way.push_back({v.series, v.number, format_iso_date(v.event.date), v.event.heading, v.event.amount, why});
  }
}

} // namespace

std::expected<SieImportPlan, std::string> plan_sie_import(Project const& project, fs::path const& sie_file,
                                                          std::optional<FiscalYear> const& year) {
  std::error_code ec;
  if (!sie_file.is_absolute()) return std::unexpected(std::format("{} är inte en hel sökväg", sie_file.string()));
  if (!fs::is_regular_file(sie_file, ec)) return std::unexpected(std::format("{} är ingen fil", sie_file.string()));
  auto books = books::read_books(project.folder);
  if (!books) return std::unexpected(books.error());
  books::Identity identity{project.name, {}};
  if (auto const entries = read_path_values(project.file)) identity.orgnr = find_value(*entries, "orgnr").value_or("");
  auto const lexed = sie::lex_file(sie_file);
  if (!lexed) return std::unexpected(lexed.error());
  auto const parsed = sie::parse_document(lexed->lines);
  auto const& document = parsed.document;
  if (document.sie_type.empty()) return std::unexpected(std::format("{} är ingen SIE-fil (#SIETYP saknas)", sie_file.string()));
  if (!parsed.errors.empty()) return std::unexpected(errors_text(parsed.errors));
  if (!document.year(0)) return std::unexpected("SIE-filen saknar räkenskapsår (#RAR 0)");
  std::optional<FiscalYear> expected;
  if (year) {
    FiscalYear const file_year{format_iso_date(document.year(0)->start), format_iso_date(document.year(0)->end)};
    if (file_year != *year) {
      if (!adjoins(*books, *year, file_year)) {
        return std::unexpected(std::format("SIE-filen gäller räkenskapsåret {} – {}, inte {}", file_year.start, file_year.end,
                                           label_of(*year)));
      }
      expected = *year;
    }
  }

  SieImportPlan plan;
  plan.sie_file = sie_file;
  plan.program = document.program_version.empty() ? document.program : std::format("{} {}", document.program, document.program_version);
  if (document.generated) plan.generated = format_iso_date(*document.generated);
  plan.alert = lexed->alert;
  auto const file_start = document.year(0)->start;
  plan.new_year = books->year_starting(file_start) == nullptr;
  plan.expected = expected;
  // A new year must join the books: no fiscal year left out between
  if (plan.new_year && !books->years.empty()) {
    auto const& oldest = books->years.front();
    auto const& latest = books->years.back();
    bool const after_latest = file_start == next_day(latest.end);
    bool const before_oldest = next_day(document.year(0)->end) == oldest.start;
    if (!after_latest && !before_oldest) {
      return std::unexpected(std::format(
          "SIE-filens räkenskapsår {} – {} ansluter inte till bokföringen, som har {} – {}. Importera först åren "
          "emellan, så att bokföringen inte får några luckor",
          format_iso_date(file_start), format_iso_date(document.year(0)->end), format_iso_date(oldest.start),
          format_iso_date(latest.end)));
    }
  }
  plan.import = books::plan_import(*books, identity, document);
  auto const& import = plan.import;
  auto const* year_in_books = books->year_starting(file_start);
  plan.year_closed = year_in_books != nullptr && year_in_books->closed;
  plan.year_after = std::ranges::any_of(books->years, [&](books::FiscalYear const& y) { return document.year(0)->end < y.start; });
  compare_series(year_in_books, document, plan);
  auto const gaps = !plan.blockers.empty(); // a gap in a series: never replaced
  // A closed year refuses the vouchers in the way; that conflict, like the others, can be replaced
  bool const closed_conflict = plan.year_closed && import.reverts() && import.errors.size() == 1;
  if (!import.errors.empty() && !closed_conflict) {
    plan.blockers.insert(plan.blockers.begin(), errors_text(import.errors));
    return plan;
  }
  if (import.reverts() || import.vouchers_forced > 0) {
    find_in_the_way(year_in_books, import, plan);
    auto const count = plan.in_the_way.size();
    plan.blockers.push_back(std::format("Bokföringen har {} {} som filen skulle ta bort eller byta ut.", count,
                                        count == 1 ? "verifikation" : "verifikationer"));
    plan.replaceable = !gaps;
    return plan;
  }
  auto changed = changed_vouchers(*books, import.books, import.year_start);
  if (!changed) {
    // The vouchers with another date or text are in the way
    if (auto const* after = import.books.year_starting(file_start); year_in_books != nullptr && after != nullptr) {
      for (auto const& v : year_in_books->vouchers) {
        auto const found = std::ranges::find_if(after->vouchers, [&](books::Voucher const& w) {
          return w.series == v.series && w.number == v.number;
        });
        if (found != after->vouchers.end() && (found->event.date != v.event.date || found->event.heading != v.event.heading)) {
          plan.in_the_way.push_back({v.series, v.number, format_iso_date(v.event.date), v.event.heading, v.event.amount,
                                     "har annat datum eller annan text i filen"});
        }
      }
    }
    plan.blockers.push_back(changed.error() + ".");
    plan.replaceable = !gaps && !plan.in_the_way.empty();
    return plan;
  }
  plan.changed = std::move(*changed);
  if (plan.blockers.empty() && books::nothing_to_import(*books, import)) {
    plan.blockers.push_back("Allt i SIE-filen finns redan i bokföringen.");
  }
  return plan;
}

namespace {

// Writes the planned books and the identity, and records the SIE file in the inventory (it stays where it is)
std::expected<InventoryEntry, std::string> write_import(Project const& project, fs::path const& sie_file,
                                                        books::ImportPlan const& import, SieUse const& use, Date today) {
  if (auto written = books::write_books(project.folder, import.books); !written) return std::unexpected(written.error());
  if (import.identity_changed) {
    auto entries = read_path_values(project.file);
    if (!entries) return std::unexpected(entries.error());
    set_value(*entries, "orgnr", import.identity.orgnr);
    set_value(*entries, "name", import.identity.name);
    if (auto written = write_path_values(project.file, *entries); !written) return std::unexpected(written.error());
  }
  auto entry = record_sie_use(project.folder, sie_file, use, today);
  if (!entry) return std::unexpected(std::format("cratchit skrev bokföringen, men kunde inte skriva förteckningen: {}", entry.error()));
  return entry;
}

} // namespace

std::expected<SieImported, std::string> import_sie(Project const& project, SieImportPlan const& plan, Date today) {
  if (!plan.possible()) return std::unexpected(plan.blockers.front());
  auto entry = write_import(project, plan.sie_file, plan.import, {"import", plan.import.vouchers_added, plan.changed.size()}, today);
  if (!entry) return std::unexpected(entry.error());
  return SieImported{std::move(*entry)};
}

std::expected<SieReplaced, std::string> replace_year_with_sie(Project const& project, SieImportPlan const& plan, Date today) {
  if (!plan.replaceable) return std::unexpected(std::string("Räkenskapsåret kan inte ersättas med den här SIE-filen"));
  auto books = books::read_books(project.folder);
  if (!books) return std::unexpected(books.error());
  books::Identity identity{project.name, {}};
  if (auto const entries = read_path_values(project.file)) identity.orgnr = find_value(*entries, "orgnr").value_or("");
  auto const lexed = sie::lex_file(plan.sie_file);
  if (!lexed) return std::unexpected(lexed.error());
  auto const document = sie::parse_document(lexed->lines).document;
  if (!document.year(0)) return std::unexpected("SIE-filen saknar räkenskapsår (#RAR 0)");
  // The year emptied of its vouchers (its drafts stay), and opened so that the file may fill it
  auto emptied = *books;
  auto const year = std::ranges::find_if(emptied.years, [&](books::FiscalYear const& y) { return y.start == document.year(0)->start; });
  if (year == emptied.years.end()) return std::unexpected(std::string("Räkenskapsåret finns inte i bokföringen"));
  auto const before = *year;
  year->vouchers.clear();
  year->closed = false;
  auto import = books::plan_import(emptied, identity, document);
  if (!import.errors.empty()) return std::unexpected(errors_text(import.errors));
  auto replaced = std::ranges::find_if(import.books.years, [&](books::FiscalYear const& y) { return y.start == before.start; });
  if (replaced != import.books.years.end()) replaced->closed = before.closed;
  auto const vouchers = replaced != import.books.years.end() ? replaced->vouchers.size() : 0;

  // What is gone, logged with its rows, for traceability
  LOG_BUSINESS("Ersätter räkenskapsåret {} – {} i projektet {} ({}) med {}: {} verifikationer försvinner, {} kommer från filen",
               format_iso_date(before.start), format_iso_date(before.end), project.name, project.uuid, plan.sie_file.string(),
               plan.in_the_way.size(), vouchers);
  for (auto const& gone : plan.in_the_way) {
    auto const found = std::ranges::find_if(before.vouchers, [&](books::Voucher const& v) {
      return v.series == gone.series && v.number == gone.number;
    });
    LOG_BUSINESS("Ersätt: {}{} {} \"{}\" {} försvinner ({})", gone.series, gone.number, gone.date, gone.heading,
                 format_amount(gone.amount), gone.why);
    if (found == before.vouchers.end()) continue;
    for (auto const& row : found->postings) {
      LOG_BUSINESS("Ersätt:   {} {} {}", row.account, format_amount(row.amount), row.date ? format_iso_date(*row.date) : std::string());
    }
  }
  auto entry = write_import(project, plan.sie_file, import, {"ersätt", vouchers, 0}, today);
  if (!entry) return std::unexpected(entry.error());
  return SieReplaced{plan.in_the_way, vouchers, std::move(*entry)};
}

} // namespace cratchit
