#pragma once

#include "core/books/import.hpp"
#include "core/inventory.hpp"
#include "core/fiscal_year.hpp"
#include "core/project.hpp"

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit {


// A voucher already in the books that the import changes: under the SIE rules
// for a registered voucher, only rows struck (#BTRANS) and rows added (#RTRANS)
struct ChangedVoucher {
  std::string series;
  std::string number;
  std::string date;    // YYYY-MM-DD
  std::string heading; // the voucher's text
  std::size_t struck = 0;
  std::size_t added = 0;
};

// "A 12  2025-06-03  Loopia: 1 rad struken, 1 rad tillagd"
std::string change_text(ChangedVoucher const& changed);

// One series of the year, in the books and in the file
struct SeriesComparison {
  std::string series;
  std::string books_last;  // the highest number in the books, empty when none
  std::string file_last;   // the highest number in the file, empty when none
  std::string new_numbers; // the file's numbers not in the books as ranges, e.g. "60–63, 65", empty when none
};

// A voucher of the books that the file would not leave as it is
struct VoucherInTheWay {
  std::string series;
  std::string number;
  std::string date;    // YYYY-MM-DD
  std::string heading;
  Amount amount = 0;
  std::string why;     // "finns inte i filen", "har annat innehåll i filen", ...
};

// What importing an SIE file into a project will do. Read from the books and the file; nothing written yet.
struct SieImportPlan {
  std::filesystem::path sie_file;
  std::string program;   // #PROGRAM name and version, e.g. "Fortnox 3.62"
  std::string generated; // #GEN as YYYY-MM-DD, or empty
  std::string alert;     // when the file was not read as CP437 (see sie::lex_file), else empty
  bool new_year = false; // the file's fiscal year is not in the books yet
  std::optional<FiscalYear> expected; // the year cratchit counted with, when the file's year adjoins it with other dates
  books::ImportPlan import;
  std::vector<ChangedVoucher> changed; // vouchers in the books with rows struck or added, by series and number
  std::vector<SeriesComparison> series; // the year's series in the books and in the file, by series
  std::vector<VoucherInTheWay> in_the_way; // vouchers of the books the file would remove or replace
  std::vector<std::string> blockers;  // why the import cannot be done, for the user; empty when it can
  bool replaceable = false; // not possible only because of vouchers in the way: the year can be replaced with the file
  bool year_closed = false; // the year in the books is closed (from take 3); it may still be replaced
  bool year_after = false;  // the books have a year after the file's, whose opening balances follow it

  bool possible() const { return blockers.empty(); }
};

// Plans importing the SIE file's fiscal year 0 into the project's books (see
// books::plan_import), with the project's name and organisation number read
// now, and compares the file with the books, series by series. Writes nothing.
//
// Error text when the comparison cannot be made: the books or the file cannot
// be read, the file is no SIE file or has errors, it is for another fiscal year
// (see year below), or its year is new and would leave a gap in the books (it
// must start the day after the latest year or end the day before the oldest).
//
// Otherwise a plan, whose blockers say why the import cannot be done, if so:
// the plan has errors (e.g. another organisation number), a series of the file
// has a gap in its numbers (the file must join the books without gaps), the
// import would change vouchers already in the books otherwise than by struck
// and added rows (another date or text), or rename, replace, drop or demote
// them (in_the_way names them), or the file has nothing new: for now cratchit
// only adds vouchers, and rows to vouchers as SIE allows. When the vouchers in
// the way are the only blockers, the plan is replaceable (see replace_year_with_sie).
//
// With year, the file must be for that fiscal year (its #RAR 0), else error text
// naming the file's year: the user imports into the year they stand in. A year
// in the books needs the same dates. A year not in the books may come with
// other dates when the file's year adjoins the books and is 1 to 18 months
// long: the first year after the latest in the books must start the day after
// it, and the year before the oldest must end the day before it. That takes in
// a first fiscal year of another length, and a change of fiscal year
// (omläggning), from an SIE file; the years after are counted from it.
std::expected<SieImportPlan, std::string> plan_sie_import(Project const& project, std::filesystem::path const& sie_file,
                                                          std::optional<FiscalYear> const& year = std::nullopt);

// What import_sie did
struct SieImported {
  InventoryEntry entry; // the SIE file's entry in the inventory of its fiscal year
};

// Imports as planned on day today: writes the books (only the files that
// change, see books::write_books), the organisation number (and name) in
// project.txt when the import gives the project one, and records the SIE file
// in the inventory of its fiscal year (see record_sie_use). The SIE file is
// never moved or changed. Error text when any step fails; what was written before stays.
std::expected<SieImported, std::string> import_sie(Project const& project, SieImportPlan const& plan, Date today);

// What replace_year_with_sie did
struct SieReplaced {
  std::vector<VoucherInTheWay> removed; // gone from the books, as the plan named them
  std::size_t vouchers = 0;             // the year's vouchers now, all from the file
  InventoryEntry entry;                 // the SIE file's entry in the inventory of its fiscal year
};

// Burns the bridges: makes the plan's fiscal year exactly what the SIE file
// holds, on day today. The year's vouchers are all taken out and the file's
// put in (see books::plan_import on the emptied year); what differed is gone.
// The year's drafts stay, except those the file books. A closed year stays
// closed. Accounts, company details and the opening balances of later years
// follow as for an import. Each removed voucher is logged with its rows, for
// traceability, and the file is recorded in the inventory as "ersätt". The
// plan must be replaceable. Error text when the books or the file cannot be
// read or written; what was written before stays.
std::expected<SieReplaced, std::string> replace_year_with_sie(Project const& project, SieImportPlan const& plan, Date today);

} // namespace cratchit
