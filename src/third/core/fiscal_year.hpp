#pragma once

#include "core/project.hpp"

#include <expected>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit {

// A fiscal year (räkenskapsår) of a project, as dates YYYY-MM-DD
struct FiscalYear {
  std::string start; // e.g. 2025-05-01
  std::string end;   // e.g. 2026-04-30

  bool operator==(FiscalYear const&) const = default;
};

// The fiscal years of the project: the `year.<start> = <end>` entries of
// <project folder>/.cratchit/books/books.txt (as written by take 3), the latest
// first. No books.txt means no fiscal years yet. Never creates or changes
// anything. Error text when books.txt cannot be read, or a date is not YYYY-MM-DD.
std::expected<std::vector<FiscalYear>, std::string> list_fiscal_years(Project const& project);

// A fiscal year as cratchit lists it: one that is in the books, or one that
// follows from them up to today and has nothing booked yet
struct ListedYear {
  FiscalYear year;
  bool in_books = false;     // a `year.` entry of books.txt; else counted on from the years before
  std::size_t vouchers = 0;  // booked in the year
};

// The fiscal years of the project from the year before the oldest in the books
// up to the one today is in (or the latest in the books, when that is later),
// the latest first, so that the first is the current year (key 0) and the rest
// go back (1, 2, ...), as #RAR 0, -1, ... in SIE. The last, the year before the
// oldest in the books (see year_before), has nothing booked: it is there so that
// the user can import an earlier year. Fiscal years exist whether booked or
// not: a year after one in the books starts the day after it and is twelve
// months (a first fiscal year may be shorter or longer, a later one is not;
// changing the fiscal year is not supported), and such a year is listed
// without being written anywhere. No years in the books gives none. Error text
// when the books cannot be read. today is YYYY-MM-DD.
std::expected<std::vector<ListedYear>, std::string> list_years_until(Project const& project, std::string_view today);

// For now cratchit handles the current fiscal year and at most this many older
// ones (beslut.md): the UI lists them as 0..9.
inline constexpr int max_older_fiscal_years = 9;

// How many fiscal years before the current one year is, counting twelve-month
// years after it (see year_after) until the one today lies in: 0 when today
// lies in year, or before its end. today is YYYY-MM-DD.
int years_before_current(FiscalYear const& year, std::string_view today);

// The fiscal year twelve months long that starts the day after year
FiscalYear year_after(FiscalYear const& year);

// The fiscal year twelve months long that ends the day before year
FiscalYear year_before(FiscalYear const& year);

// The length of the year in whole months, e.g. 12 for 2025-05-01 – 2026-04-30
int months_of(FiscalYear const& year);

// How the year is named in the UI: "2025" when it starts and ends in the same
// calendar year, else "2025/26" (and "1999/2000" across a century)
std::string label_of(FiscalYear const& year);

} // namespace cratchit
