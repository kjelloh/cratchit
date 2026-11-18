#include "core/inventory.hpp"

#include "core/books/books.hpp"
#include "core/csv.hpp"
#include "core/sha256.hpp"
#include "core/sie/document.hpp"
#include "core/sie/file.hpp"
#include "core/sie_summary.hpp"

#include <algorithm>
#include <format>
#include <fstream>
#include <iterator>
#include <system_error>

namespace cratchit {

namespace fs = std::filesystem;

namespace {

CsvRow const header{"sha256", "kind", "name", "path", "size", "read", "use", "generated", "program",
                    "year_start", "year_end", "status", "vouchers_added", "vouchers_changed"};

CsvRow row_of(InventoryEntry const& e) {
  return {e.sha256, e.kind, e.name, e.path, e.size, e.read, e.use, e.generated, e.program,
          e.year_start, e.year_end, e.status, e.vouchers_added, e.vouchers_changed};
}

InventoryEntry entry_of(CsvRow const& r) {
  return {r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8], r[9], r[10], r[11], r[12], r[13]};
}

} // namespace

std::string stored_path(fs::path const& file, fs::path const& project_folder) {
  std::error_code ec;
  auto const relative = fs::relative(file, project_folder, ec);
  auto const path = (ec || relative.empty()) ? file : relative;
  auto const text = path.generic_u8string();
  return std::string(text.begin(), text.end());
}

namespace {

std::string utf8_name(fs::path const& file) {
  auto const text = file.filename().u8string();
  return std::string(text.begin(), text.end());
}

} // namespace

fs::path inventory_file(fs::path const& project_folder) { return project_folder / ".cratchit" / "inventory.csv"; }

fs::path year_inventory_file(fs::path const& project_folder, Date year_start) {
  return books::books_folder(project_folder) / format_iso_date(year_start) / "inventory.csv";
}

std::expected<std::vector<InventoryEntry>, std::string> read_inventory_file(fs::path const& file) {
  std::error_code ec;
  if (!fs::exists(file, ec)) return std::vector<InventoryEntry>{};
  auto const rows = read_csv(file);
  if (!rows) return std::unexpected(rows.error());
  std::vector<InventoryEntry> entries;
  for (std::size_t i = 0; i < rows->size(); ++i) {
    if (i == 0 && (*rows)[i] == header) continue;
    if ((*rows)[i].size() != header.size()) {
      return std::unexpected(std::format("{}: rad {} har {} fält, inte {}", file.string(), i + 1, (*rows)[i].size(),
                                         header.size()));
    }
    entries.push_back(entry_of((*rows)[i]));
  }
  return entries;
}

std::expected<void, std::string> add_to_inventory(fs::path const& file, InventoryEntry const& entry) {
  auto entries = read_inventory_file(file);
  if (!entries) return std::unexpected(entries.error());
  entries->push_back(entry);
  std::error_code ec;
  fs::create_directories(file.parent_path(), ec);
  if (ec) return std::unexpected(std::format("Kan inte skapa {}: {}", file.parent_path().string(), ec.message()));
  std::vector<CsvRow> rows{header};
  for (auto const& e : *entries) rows.push_back(row_of(e));
  return write_csv(file, rows);
}

std::expected<std::vector<InventoryEntry>, std::string> read_inventory(fs::path const& project_folder,
                                                                      std::optional<Date> year_start) {
  auto entries = read_inventory_file(inventory_file(project_folder));
  if (!entries) return entries;
  if (!year_start) return entries;
  auto const of_year = read_inventory_file(year_inventory_file(project_folder, *year_start));
  if (!of_year) return std::unexpected(of_year.error());
  // The year's entries override the project's for the same file
  std::erase_if(*entries, [&](InventoryEntry const& e) {
    return std::ranges::any_of(*of_year, [&](InventoryEntry const& y) { return y.sha256 == e.sha256; });
  });
  entries->insert(entries->end(), of_year->begin(), of_year->end());
  return entries;
}

std::expected<InventoryEntry, std::string> record_sie_use(fs::path const& project_folder, fs::path const& sie_file,
                                                          SieUse const& use, Date today) {
  auto const sha256 = sha256_of_file(sie_file);
  if (!sha256) return std::unexpected(sha256.error());
  std::ifstream in(sie_file, std::ios::binary);
  std::vector<std::uint8_t> const bytes{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
  auto const lexed = sie::lex_bytes(bytes);
  if (!lexed) return std::unexpected(std::format("{}: {}", sie_file.string(), lexed.error()));
  auto const document = sie::parse_document(lexed->lines).document;
  auto const year = document.year(0);
  if (!year) return std::unexpected(std::format("{} saknar räkenskapsår (#RAR 0)", sie_file.string()));
  auto const summary = summarize_sie_file(sie_file);
  auto const status = summary ? status_of(*summary, today) : std::nullopt;

  InventoryEntry entry;
  entry.sha256 = *sha256;
  entry.kind = "sie";
  entry.name = utf8_name(sie_file);
  entry.path = stored_path(sie_file, project_folder);
  entry.size = std::to_string(bytes.size());
  entry.read = format_iso_date(today);
  entry.use = use.use;
  entry.generated = document.generated ? format_iso_date(*document.generated) : std::string();
  entry.program = document.program_version.empty() ? document.program
                                                   : std::format("{} {}", document.program, document.program_version);
  entry.year_start = format_iso_date(year->start);
  entry.year_end = format_iso_date(year->end);
  entry.status = status ? status_text(SieStatus{status->status, false}) : std::string();
  entry.vouchers_added = std::to_string(use.vouchers_added);
  entry.vouchers_changed = std::to_string(use.vouchers_changed);
  if (auto added = add_to_inventory(year_inventory_file(project_folder, year->start), entry); !added) {
    return std::unexpected(added.error());
  }
  return entry;
}

std::expected<InventoryEntry, std::string> record_statement_use(fs::path const& project_folder,
                                                                fs::path const& statement_file, Date year_start,
                                                                Date year_end, Date today) {
  auto const sha256 = sha256_of_file(statement_file);
  if (!sha256) return std::unexpected(sha256.error());
  std::error_code ec;
  auto const size = fs::file_size(statement_file, ec);
  if (ec) return std::unexpected(std::format("Kan inte läsa {}: {}", statement_file.string(), ec.message()));
  InventoryEntry entry;
  entry.sha256 = *sha256;
  entry.kind = "kontoutdrag";
  entry.name = utf8_name(statement_file);
  entry.path = stored_path(statement_file, project_folder);
  entry.size = std::to_string(size);
  entry.read = format_iso_date(today);
  entry.use = "utkast";
  entry.year_start = format_iso_date(year_start);
  entry.year_end = format_iso_date(year_end);
  if (auto added = add_to_inventory(year_inventory_file(project_folder, year_start), entry); !added) {
    return std::unexpected(added.error());
  }
  return entry;
}

} // namespace cratchit
