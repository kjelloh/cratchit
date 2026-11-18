#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit::ink2 {

// A field of SKV's forms INK2 (page 1), INK2R and INK2S as SKV's field tables give it
// (INK2*_SKV2002-34-01-26-04.xls, "Fältnamn på ..."): which form, its SRU code, its number
// on the form, its text, and SKV's mark of how it counts.
struct SkvField {
  enum class Type { amount, mark };

  std::string form; // INK2, INK2R or INK2S
  std::string sru;  // e.g. 7653
  std::string ink2; // e.g. 4.3c; empty for the yes/no marks
  std::string text;
  // '+' adds to and '-' takes from the result (INK2R) or the överskott (INK2S); '*' is neither
  // (balance sheet, INK2 page 1, INK2S 4.17-4.22); ' ' for a mark
  char sign = ' ';
  Type type = Type::amount; // a mark is given as X
};

// The fields of a CSV with the header form;sru;ink2;text;sign;type, in SKV's order. Error text
// names the row.
std::expected<std::vector<SkvField>, std::string> parse_skv_fields(std::string_view csv_text);

// The fields of one form, in order
std::vector<SkvField> fields_of(std::vector<SkvField> const& fields, std::string_view form);

// The SKV field tables built into the program
std::string_view skv_fields_text();
constexpr std::string_view skv_fields_file = "resources/ink2/skv_ink2_2026P4.csv";

} // namespace cratchit::ink2
