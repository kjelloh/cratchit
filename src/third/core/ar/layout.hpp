#pragma once

#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cratchit::ar {

// BAS accounts as the mapping writes them, e.g. "1590-1619, 1630-1659 eller 1680-1689"
// or "4900-4999 (förutom 4910-4931 och 4980-4989)". An exclusion applies to the whole set.
struct AccountSet {
  std::vector<std::pair<int, int>> included; // first to last, both included
  std::vector<std::pair<int, int>> excluded;

  // Whether a four digit account number is in the set; false for any other text
  bool contains(std::string_view account) const;
};

// Numbers and ranges separated by ",", "eller" or "och", each optionally
// followed by "(förutom ...)" of the same. Error text quotes the set.
std::expected<AccountSet, std::string> parse_account_set(std::string_view text);

// What a line of an årsredovisning is
enum class LineKind {
  heading, // a title over the lines below it
  item,    // a post with an amount, from its accounts
  sum,     // the items since the last heading at its level or above
  outside, // accounts that are not on the årsredovisning at all
};

// A line of the resultaträkning or balansräkning, in order
struct Line {
  int level = 0; // 1 for the statements, deeper for their parts; 0 for outside
  LineKind kind = LineKind::item;
  AccountSet accounts; // for items and outside only
  std::string text;
};

// The lines of a mapping CSV with the columns Nivå;Typ;BAS Range;ÅR and a
// header row. Typ is Rubrik, Post, Summa or Utanför. Error text names the row.
std::expected<std::vector<Line>, std::string> parse_layout(std::string_view csv_text);

// A mapping built into the program, resources/ar/bas_to_ar_<year>.csv
struct BuiltInLayout {
  int year = 0;          // the BAS year it is made for
  std::string_view file; // e.g. resources/ar/bas_to_ar_2024.csv
  std::string_view text;
};

// The built-in mappings, oldest first (BAS_TO_AR_YEARS in CMakeLists.txt)
std::vector<BuiltInLayout> const& built_in_layouts();

// The BAS year of the books when none is given
constexpr int default_bas_year = 2024;

// The mapping for books that follow BAS bas_year: the newest built-in mapping
// not newer than bas_year, or nothing when all are newer
std::optional<BuiltInLayout> layout_for(int bas_year);

// For which BAS years layout is chosen, e.g. "BAS 2024-2025" or "BAS 2026 and later"
std::string bas_years_of(BuiltInLayout const& layout);

// The text of the mapping for default_bas_year
std::string_view layout_text();

} // namespace cratchit::ar
