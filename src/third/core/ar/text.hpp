#pragma once

#include "core/ar/disposition.hpp"
#include "core/ar/equity.hpp"
#include "core/ar/notes.hpp"
#include "core/ar/overview.hpp"
#include "core/ar/requirements.hpp"
#include "core/ar/statement.hpp"
#include "core/ar/trace.hpp"
#include "core/sie/document.hpp"

#include <map>
#include <string>
#include <vector>

namespace cratchit::ar {

// Kronor with a space between thousands, e.g. "-84 193"
std::string format_kronor(Kronor kronor);

// Förändringar i eget kapital as plain text: a column per post and the total,
// a row per change, each amount rounded to kronor on its own
std::string format_equity_changes(EquityChanges const& changes);

// The resultaträkning and the balansräkning as plain text, one column per
// year (index 0 first): the shown lines, indented by level, long texts wrapped.
// With notes (post text to note number, see note_references), the balansräkning
// has a Not column with the number of each post's note.
std::string format_statements(Statement const& statement, std::vector<sie::FiscalYear> const& years,
                              std::map<std::string, std::string> const& notes = {});

// Flerårsöversikt as plain text: a column per year, amounts in tkr, soliditet in
// percent; then the missing earlier years and the comment, TBD as check lists them
std::string format_overview(Overview const& overview, std::vector<sie::FiscalYear> const& years, Check const& check);

// Resultatdisposition as plain text: the funds with their sum, then the
// proposal's rows marked TBD [n] from check, as the amounts are not in the SIE file
std::string format_disposition(Disposition const& disposition, Check const& check);

// Noter as plain text, for fiscal year 0 (years.front()): Not 1 Redovisningsprinciper
// (with the depreciation periods TBD when check lists them), Not 2 Medelantal
// anställda when check lists it, then each note of notes with its sections and sum
std::string format_notes(Notes const& notes, std::vector<sie::FiscalYear> const& years, Check const& check);

// The front of the årsredovisning: name, orgnr, that it is an årsredovisning for
// fiscal year 0, PRELIMINÄR when check says the books are not closed, and the
// sentence on the board, the currency and the unit
std::string format_front(sie::Document const& document, std::vector<sie::FiscalYear> const& years,
                         Check const& check);

// The start of the förvaltningsberättelse: Verksamheten with Allmänt om
// verksamheten (säte) and Väsentliga händelser under räkenskapsåret, TBD as check lists them
std::string format_business(Check const& check);

// The signature page: whose årsredovisning for which year, then place, date and signatures (TBD)
std::string format_signatures(sie::Document const& document, std::vector<sie::FiscalYear> const& years,
                              Check const& check);

// The K2 check: the rules and the mapping it was made with, what is missing
// ([n], rule, what to provide, template heading), then what to confirm
std::string format_check(Check const& check, std::string_view mapping = {});

// The trace of the årsredovisning as plain text, in the order of the report:
// Flerårsöversikt and Resultatdisposition (the lines they are computed from),
// Förändringar i eget kapital (accounts of the first and last row, vouchers of
// the changes), the resultaträkning and balansräkning (accounts and year 0
// vouchers under each shown item), then the notes (accounts of the first and
// last row of each section, vouchers of its changes)
std::string format_trace(Overview const& overview, Disposition const& disposition, EquityChanges const& equity,
                         Statement const& statement, Notes const& notes, sie::Document const& document,
                         std::vector<sie::FiscalYear> const& years);

} // namespace cratchit::ar
