#pragma once

#include "core/date.hpp"
#include "core/ink2/form.hpp"
#include "core/ink2/mapping.hpp"
#include "core/ink2/skv.hpp"
#include "core/ink2/tax.hpp"

#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit::ink2 {

// What the return is of
struct Header {
  std::string company;
  std::string orgnr;
  Date start; // fiscal year 0
  Date end;
  std::string status; // e.g. "complete" or "PRELIMINÄR: the books are not closed; 1 item TBD"
};

// What to show after a field's amount, by SRU code, e.g. 7654 -> "TBD [2]"
using Markers = std::map<std::string, std::string>;

// The head: company and orgnr, the fiscal year with the fields 7011 and 7012, and the status
std::string format_head(Header const& header);

// INK2R as text: for each INK2R line of the mapping its number, text, SRU code(s), amount(s) in
// whole kronor (blank when no account is in the field) and marker, then the sums.
std::string format_form(std::vector<Line> const& lines, Form const& form, Markers const& markers = {});

// A form as SKV's fields give it (e.g. INK2S), under title: each field's number, text, SRU code,
// SKV's + or -, its amount (blank when not given) or X for a mark given, and its marker
std::string format_skv_form(std::string_view title, std::vector<SkvField> const& fields,
                            std::map<std::string, Kronor> const& amounts, std::set<std::string> const& marks,
                            Markers const& markers = {});

// The whole return as text, in the order of the forms: the head, INK2 page 1, INK2R and INK2S
std::string format_return(Header const& header, std::vector<Line> const& lines, std::vector<SkvField> const& skv,
                          Form const& form, TaxReturn const& tax, Markers const& markers);

// Whole kronor grouped by thousands with spaces, e.g. -1 234 567
std::string format_kronor(Kronor kronor);

} // namespace cratchit::ink2
