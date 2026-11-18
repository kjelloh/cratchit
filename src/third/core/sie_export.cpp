#include "core/sie_export.hpp"

#include "core/books/import.hpp"
#include "core/date.hpp"
#include "core/path_value.hpp"
#include "core/sie/document.hpp"
#include "core/sie/file.hpp"
#include "core/sie/writer.hpp"

#include <algorithm>
#include <format>
#include <optional>

namespace cratchit {

namespace {

// The books of the project, the index of the year in them and who they are for, read now
struct Loaded {
  books::Books books;
  std::size_t index = 0;
  books::Identity identity;
};

std::expected<Loaded, std::string> load(Project const& project, FiscalYear const& year) {
  auto books = books::read_books(project.folder);
  if (!books) return std::unexpected(books.error());
  auto const start = parse_iso_date(year.start);
  auto const end = parse_iso_date(year.end);
  if (!start || !end) return std::unexpected(std::format("{} – {} är inget räkenskapsår", year.start, year.end));
  auto found = std::ranges::find(books->years, *start, &books::FiscalYear::start);
  if (found == books->years.end()) {
    // A year not booked yet (see list_years_until): exported with nothing in it, here in memory only
    auto const at = std::ranges::find_if(books->years, [&](books::FiscalYear const& y) { return y.start > *start; });
    found = books->years.insert(at, books::FiscalYear{*start, *end, {}, {}});
  }
  auto const index = static_cast<std::size_t>(found - books->years.begin());
  books::Identity identity{project.name, {}};
  if (auto const entries = read_path_values(project.file)) identity.orgnr = find_value(*entries, "orgnr").value_or("");
  return Loaded{std::move(*books), index, std::move(identity)};
}

std::expected<Exported, std::string> write(Project const& project, Loaded const& loaded, books::ExportKind kind,
                                           std::string_view today,
                                           std::optional<std::vector<std::size_t>> const& only = std::nullopt) {
  auto const& year = loaded.books.years[loaded.index];
  auto const generated = parse_iso_date(today).value_or(year.end);
  auto const document = books::export_document(loaded.books, loaded.identity, loaded.index, generated, only);
  auto const encoded = sie::cp437_of(sie::text_of(document));
  auto const written = books::write_export(project.folder, year, kind, encoded.bytes);
  if (!written) return std::unexpected(written.error());
  return Exported{*written, document.vouchers.size(), encoded.unmapped};
}

} // namespace

std::expected<Exported, std::string> export_all(Project const& project, FiscalYear const& year, std::string_view today) {
  auto const loaded = load(project, year);
  if (!loaded) return std::unexpected(loaded.error());
  return write(project, *loaded, books::ExportKind::all, today);
}

std::expected<SieComparison, std::string> compare_with_sie(Project const& project, FiscalYear const& year,
                                                           std::filesystem::path const& sie_file) {
  auto const loaded = load(project, year);
  if (!loaded) return std::unexpected(loaded.error());
  std::error_code ec;
  if (!std::filesystem::is_regular_file(sie_file, ec)) return std::unexpected(std::format("{} är ingen fil", sie_file.string()));
  auto const lexed = sie::lex_file(sie_file);
  if (!lexed) return std::unexpected(lexed.error());
  auto const parsed = sie::parse_document(lexed->lines);
  if (parsed.document.sie_type.empty()) return std::unexpected(std::format("{} är ingen SIE-fil (#SIETYP saknas)", sie_file.string()));
  if (!parsed.errors.empty()) {
    return std::unexpected(std::format("SIE-filen har {} fel, till exempel: {}", parsed.errors.size(), parsed.errors.front()));
  }
  auto const diff = books::diff_with(loaded->books, loaded->index, loaded->identity, parsed.document);
  if (!diff) return std::unexpected(diff.error());
  SieComparison comparison;
  comparison.sie_file = sie_file;
  auto const& document = parsed.document;
  comparison.program = document.program_version.empty() ? document.program
                                                        : std::format("{} {}", document.program, document.program_version);
  if (document.generated) comparison.generated = format_iso_date(*document.generated);
  auto const& vouchers = loaded->books.years[loaded->index].vouchers;
  for (auto const i : diff->books_only) comparison.books_only.push_back(vouchers[i]);
  comparison.books_only_index = diff->books_only;
  comparison.file_only = diff->file_only;
  return comparison;
}

std::expected<Exported, std::string> export_difference(Project const& project, FiscalYear const& year,
                                                       SieComparison const& comparison, std::string_view today) {
  if (!comparison.file_only.empty()) {
    return std::unexpected("SIE-filen har verifikationer som inte finns i räkenskapsåret, eller som ser annorlunda ut där. Program som Fortnox kan bara "
                           "lägga till verifikationer, så de måste stämma först");
  }
  if (comparison.books_only.empty()) return std::unexpected("Alla verifikationer i räkenskapsåret finns redan i SIE-filen");
  auto const loaded = load(project, year);
  if (!loaded) return std::unexpected(loaded.error());
  return write(project, *loaded, books::ExportKind::diff, today, comparison.books_only_index);
}

} // namespace cratchit
