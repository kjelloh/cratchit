#pragma once

#include "core/ink2/accounts.hpp"

#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit::ink2 {

// The form of an SRU code: INK2R for 72xx-75xx, INK2S for 76xx-77xx, else empty
std::string_view form_of(std::string_view sru);

// A field of INK2R or INK2S: its SRU code (SKV's fältkod) and the BAS accounts in it
struct Field {
  std::string sru;     // e.g. 7215
  std::string bas;     // the accounts as the mapping writes them, e.g. "12xx (exkl. 128x)"
  AccountSet accounts;
  // How the field shows the accounts' amount (debit positive in SIE): 1 as booked, -1 negated.
  // By the SRU code: 72xx (tillgångar) and 75xx (the minus fields of the resultaträkning, costs)
  // as booked; 73xx (eget kapital och skulder) and 74xx (the plus fields, income) negated. The
  // year's result (3.26 and 3.27, conditions profit and loss) is the exception: 899x carries it
  // with the opposite sign, so the profit field shows 899x as booked and the loss field negated.
  // INK2S likewise by SKV's sign of the field: 76xx (+, e.g. costs not to deduct) as booked,
  // 77xx (-, e.g. income not to tax) negated.
  int sign = 1;

  bool in_balance_sheet() const { return sru.starts_with("72") || sru.starts_with("73"); }
};

// A line of the form: a heading, or a row with its number on the form (e.g. 2.4) and its
// fields, one or, when the amount goes to a plus or a minus field by its sign, two
struct Line {
  enum class Kind { heading, row };
  Kind kind = Kind::heading;
  std::string form; // INK2R or INK2S: of the fields of a row; of the next row for a heading
  std::string ink2; // e.g. 2.4; rows only
  std::string text;
  std::vector<Field> fields;
};

// The lines of a mapping CSV with the columns bas;sru;ink2;text (further columns empty) and
// that header. A row with only text is a heading, one with all four a row of the form, and
// one with only bas and sru the second field of the row before it. Blank rows are skipped.
// Error text names the row.
std::expected<std::vector<Line>, std::string> parse_mapping(std::string_view csv_text);

// The mapping built into the program
std::string_view mapping_text();
constexpr std::string_view mapping_file = "resources/ink2/bas_2023_to_ink2_2026.csv";

} // namespace cratchit::ink2
