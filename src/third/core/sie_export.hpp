#pragma once

#include "core/books/export.hpp"
#include "core/fiscal_year.hpp"
#include "core/project.hpp"

#include <cstddef>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace cratchit {

// What an export wrote
struct Exported {
  std::filesystem::path file; // in the project folder, never overwriting (see books::write_export)
  std::size_t vouchers = 0;
  std::size_t unmapped = 0; // characters CP437 does not have, written as <U+XXXX>
};

// Exports the whole fiscal year ("Allt") as an SIE 4E file in the project folder,
// sie_<year>_allt.se: identification, accounts, dimensions, objects, the balances
// cratchit calculates and every voucher (see books::export_document). The books,
// the project's name and orgnr are read now. A year not in the books yet (see
// list_years_until) is exported as an empty year, its opening balances carried
// from the year before. Error text when they cannot be read, or the file cannot
// be written.
std::expected<Exported, std::string> export_all(Project const& project, FiscalYear const& year, std::string_view today);

// The fiscal year compared with an SIE file of the same year (see books::diff_with)
struct SieComparison {
  std::filesystem::path sie_file;
  std::string program;   // #PROGRAM name and version of the file, e.g. "Fortnox 3.0"
  std::string generated; // #GEN as YYYY-MM-DD, or empty
  std::vector<books::Voucher> books_only;     // in cratchit, not in the file
  std::vector<std::size_t> books_only_index; // the same, as indexes into the year's vouchers
  std::vector<books::Voucher> file_only;      // in the file, not in cratchit
};

// Compares the fiscal year with the SIE file. The difference is counted against
// the file, not against the other program itself: it is only right when the
// file shows the books in the program we export to as they are now, i.e. an
// export made just before. cratchit cannot tell whether the file is current;
// the UI warns when it was generated before today or has no date.
// Error text when the books or the
// file cannot be read, the file has errors, or is for another organisation
// number or fiscal year.
std::expected<SieComparison, std::string> compare_with_sie(Project const& project, FiscalYear const& year,
                                                           std::filesystem::path const& sie_file);

// Exports the vouchers in cratchit that are not in the compared SIE file
// ("Skillnad") as an SIE 4E file in the project folder, sie_<year>_skillnad.se,
// without balances. Error text when there is nothing to export, when the file
// has vouchers that are not in cratchit or look otherwise there, or when the
// books cannot be read or the file written.
//
// Skillnad is there to export safely to programs like Fortnox (others may
// import the same way). Fortnox imports an SIE file
// by adding its vouchers: it keeps the series but numbers them anew, and it
// cannot change or remove vouchers it has from what the file holds. So a file
// for Fortnox must hold only the vouchers Fortnox does not have yet: vouchers
// are matched on date and counted postings, not series and number (Fortnox
// renumbers), and any voucher of the file that cratchit lacks or holds
// otherwise stops the export, as Fortnox could not take the change. "Allt" is
// not for Fortnox: importing it would double every voucher Fortnox has.
std::expected<Exported, std::string> export_difference(Project const& project, FiscalYear const& year,
                                                       SieComparison const& comparison, std::string_view today);

} // namespace cratchit
