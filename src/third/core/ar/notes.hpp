#pragma once

#include "core/amount.hpp"
#include "core/ar/statement.hpp"
#include "core/sie/document.hpp"

#include <map>
#include <string>
#include <vector>

namespace cratchit::ar {

// A row of a note section, in öre as booked (debit positive) for fiscal year 0
struct NoteRow {
  std::string text;
  Amount amount = 0;
  Balances accounts;                         // for the first and last row: the balances behind it
  std::vector<sie::Voucher const*> vouchers; // for a change: the vouchers behind it (in the document), by date
};

// A post under a note heading, e.g. Anskaffningsvärden, as rows from opening to closing
struct NoteSection {
  std::string text;
  AccountSet accounts;
  std::vector<NoteRow> rows; // the opening balance, the changes with an amount, the closing balance
};

// A note: a heading under Noter, its sections, and the sum after them (Redovisat värde)
struct Note {
  std::string title;              // e.g. Not 3 Inventarier, verktyg och installationer
  std::vector<NoteSection> sections;
  std::string sum_text;           // e.g. Redovisat värde
  Amount sum = 0;                 // the sections' closing balances
};

struct Notes {
  std::vector<Note> notes;         // the shown ones
  std::vector<std::string> errors; // a section that does not add up, a sum that differs from its post, an unknown section
};

// The shown notes of statement, for fiscal year 0. A section's rows come from
// #IB 0 of its accounts, the year's vouchers on them and #UB 0, by its text:
// - Anskaffningsvärden: Inköp, and Försäljningar och utrangeringar
// - Avskrivningar: Återförda avskrivningar and Årets avskrivningar
// - Nedskrivningar: Återförda nedskrivningar and Årets nedskrivningar
// A voucher is a sale or utrangering (the first of the two) when it also posts
// to another section of the note or to 3970-3979 or 7970-7979. The opening
// balance and changes must add up to the closing balance, and a note whose
// title is "Not <n> <post>" must sum to that post of the balansräkning.
Notes build_notes(Statement const& statement, sie::Document const& document);

// The note number of each post with a note titled "Not <n> <post>", e.g. {"Inventarier, verktyg och installationer", "3"}
std::map<std::string, std::string> note_references(Notes const& notes);

} // namespace cratchit::ar
