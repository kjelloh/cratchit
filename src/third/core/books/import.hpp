#pragma once

#include "core/books/books.hpp"
#include "core/sie/document.hpp"

#include <cstddef>
#include <expected>
#include <compare>
#include <filesystem>
#include <set>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace cratchit::books {

// Who the books are for: the official name and organisation number of the project
struct Identity {
  std::string name;
  std::string orgnr; // empty until the first import sets it
};

// A voucher by the start of its fiscal year, its series and its number
struct VoucherKey {
  Date year;
  std::string series;
  std::string number;

  auto operator<=>(VoucherKey const&) const = default;
};

// What importing an SIE file into the books would do
struct ImportPlan {
  std::vector<std::string> errors; // when any, nothing may be imported
  std::vector<std::string> notes;  // worth knowing, but do not stop the import
  Books books;                     // the books after the import
  Identity identity;               // the project's identity after the import
  bool identity_changed = false;
  Date year_start;
  Date year_end;
  std::size_t vouchers_added = 0;
  std::size_t vouchers_changed = 0; // in the books, with rows struck or added in the file
  std::size_t vouchers_forced = 0;  // in the books, replaced by the file's against the history rule (see Forcing)
  std::vector<std::string> forced;  // each forced voucher with its old and new version, for the log and the record
  std::size_t forceable = 0;        // the errors a forced import would take (each also in errors)
  std::size_t vouchers_known = 0;   // already in the books, identical
  std::size_t accounts_added = 0;
  std::size_t accounts_changed = 0;
  // Making the file the truth of its year: what leaves the books or changes against the history rule
  std::vector<std::string> renamed;  // the same voucher under another name, e.g. "D 5 as D 7"
  std::vector<std::string> replaced; // in the books, replaced by the file's voucher with the same series and number
  std::vector<std::string> dropped;  // in the books, a correction and the voucher it corrects, both left out
  std::vector<std::string> demoted;  // in the books, now drafts
  std::vector<std::string> reverted; // each of them in full, for the log
  std::size_t registered_after_file = 0; // replaced, dropped and demoted ones registered after the file's #GEN
  // Drafts the file brings as vouchers (the same date and counted postings), taken off the drafts,
  // e.g. `2026-07-03 "AVGIFTER NORDEA" as A 12`
  std::vector<std::string> drafts_booked;

  // True when there are errors and a forced import would take every one of them
  bool can_force() const { return !errors.empty() && forceable == errors.size(); }
  // True when the import makes the file the truth: a voucher in the books is renamed, replaced, dropped or demoted
  bool reverts() const { return !renamed.empty() || !replaced.empty() || !dropped.empty() || !demoted.empty(); }
};

// The vouchers that came with SIE imports (see imported_vouchers), and whether to force: a voucher of an open year
// that differs from the one in the books against the history rule is then taken from the file, when the one in the
// books was imported (one entered in cratchit is another event with the same number, never forced)
struct Forcing {
  std::set<VoucherKey> imported;
  bool force = false;
};

// Plans importing a parsed SIE 4E file (see sie::check_document for the file's own
// errors and notes, which are included). The file's fiscal year 0 is imported:
//
// - Identity: the first import (no orgnr yet) sets the organisation number and
//   name from #ORGNR and #FNAMN. Later, a different orgnr or name is an error.
// - Fiscal year: a new year must not overlap a year in the books; a year with
//   the same start must have the same end.
// - Vouchers: the file is the truth of its year 0. One already in the books (same
//   series and number) is skipped when it means the same: a row dated as its
//   voucher counts as having no date, a zero quantity as none (Fortnox writes
//   them so). It is taken from the file (and
//   noted) when only history is added: the same registration and signature (the
//   date and text may be edited), each row of the books in its place unchanged or
//   struck (#BTRANS), a struck row still struck, and any further rows added
//   (#RTRANS). With forcing, a voucher that differs otherwise and was imported, in
//   an open year, is taken from the file (see Forcing). The rest of the year's
//   vouchers make the file the truth (see ImportPlan::reverts), in this order:
//   a voucher in the books with the same date and counted postings (account and
//   amount) as a voucher of the file is the same voucher under another name, the
//   file's replaces it (renamed; the same text first, then the lowest number);
//   one with the same series and number as a voucher of the file is replaced by
//   it; of those left, a voucher with the same date as another and its counted
//   postings turned (a correction and what it corrects; not a credit note on
//   another day) is dropped with it; the others become
//   drafts with their counted postings (demoted), noted on their
//   registration day. The file's other vouchers are added. Any change in a
//   closed year is an error. Each voucher the file puts in place (added,
//   renamed or replaced) takes one draft with the same date and
//   counted postings off the year's drafts (drafts_booked).
//   Row dates the file is silent about are kept: a row of a voucher the file
//   puts in place (or leaves as it is) that has no date in the file takes the
//   date of a row of the books' voucher with the same account and amount, when
//   that date is the row's own (not its voucher's date), e.g. a bank's date from
//   a statement entry. A date the file gives wins. Each kept date is noted.
// - Accounts, dimensions, objects and company details: new ones are added;
//   changes are taken (and noted) only from a file whose year is the latest in
//   the books, so an older file does not undo newer names.
// - Balances are cratchit's own: a year directly after another opens with its
//   closing balances. The file's opening and closing balances (#IB, #UB, #RES,
//   also of earlier years in the books) are only checked; each difference is
//   noted with the vouchers that may explain it (those with that amount on the
//   account), and so is any opening balance the import changes in other years.
ImportPlan plan_import(Books const& books, Identity const& identity, sie::Document const& document,
                       Forcing const& forcing = {});

// True when the plan (without errors) adds nothing to the books: no new voucher, account or identity, and its year
// is already there
bool nothing_to_import(Books const& books, ImportPlan const& plan);

// The voucher of an SIE #VER; SIE gives no amount, so it is the debit total
Voucher voucher_of(sie::Voucher const& voucher);

// The counted postings (not removed) as account and amount, sorted, turned when negated: with the date, what makes
// two vouchers the same
std::vector<std::pair<std::string, Amount>> counted_rows(Voucher const& voucher, bool negated = false);

// A voucher in full for the log, e.g. `2026-04-20 "Loopia" registered 2026-07-15: row 1920 -323.75, row 2440 323.75`
std::string voucher_text(Voucher const& voucher);

} // namespace cratchit::books
