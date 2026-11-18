#pragma once

#include "core/books/books.hpp"
#include "core/fiscal_year.hpp"
#include "core/project.hpp"
#include "pyroceph/tui/state.hpp"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace cratchit::ui {

// The pairs Samma? shows at a time, three lines each
inline constexpr std::size_t same_page_pairs = 6;

// Utkast: the fiscal year's drafts, the events on their way into the books, as
// Bokfört lists the vouchers: newest first, a page of booked_page_rows at a
// time (see draft_rows and pyroceph::tui::Pager), with a heading "Utkast
// <year>: utkast 1–20 av 31" ("Utkast <year>: inga utkast." for none) and "…"
// above and below the page when there are more. "<" and ">" turn a page in
// place; at either end they do nothing. The drafts are read again when the
// user comes back. Below the list, what was just done (e.g. the drafts a
// statement gave), until the user goes on. 3 Läs kontoutdrag leads to
// make_read_statement_menu.
// 0 Beskriv händelser: the user types one event after the other, each on one
// line (see parse_event): an error is shown below the line, which is kept to
// correct. When the amount can be read in more than one way, "Belopp?" lists
// the readings to choose with 1, 2, ... (- goes back to the line, kept). A
// clear line, or a reading chosen, adds the draft (add_event_draft) at once,
// without confirmation, and the user is back in Beskriv händelser with an
// empty line, the drafts made there listed under "Skapade nu:" (date, amount,
// heading; no ids, as they change). - goes back to Utkast, which tells
// "cratchit skapade 3 utkast." (or "cratchit skapade 1 utkast: <heading>.").
// 1 Samma? (beslut 2026-11-17, as take 3's matches): the possible pairs of
// drafts that may be the same event, or a draft already in the books as a
// voucher (find_same_events), numbered, the likely ones first, same_page_pairs
// at a time. A number and Enter opens the pair, named by its number: both in
// full and what holds them together. Two drafts: 0 Samma! makes them one at
// once (merge_drafts; the pair shown in full is the confirmation), with the
// event of the one that has it (at most one has). A draft and a
// voucher: 9 Glöm forgets the draft (9 again confirms). Done, back to Samma?,
// which finds the pairs again and lists what was done; Utkast tells it too.
// 2 Fullborda utkast: the drafts one at a time from the oldest, < and > to
// browse, with the options of the draft's view (0 Händelse, 1 Konteringar,
// 2 Bokför when ready, 9 Glöm or Inte samma).
// 5 Gör bokfört till utkast leads to make_unbook_menu (also 0 in Bokfört).
// 4 Läs pdf-underlag: the user types the path of a folder, starting in the
// project folder; Tab completes folders and shows the folder's pdf files as
// information. Enter on a folder reads its pdf files with walk_pdf (see
// pdf_status) and leads to "Pdf-underlag": the files numbered, each with a
// status line telling whether it has text, a page at a time with < and >.
// Anything else is refused with the reason, and the path kept.
// Läs kontoutdrag and Läs pdf-underlag start in the folder chosen last in
// either (beslut 2026-11-17: statements and documents often lie together).
// Opened with 0 in Årsflöde. No input.
std::unique_ptr<pyroceph::tui::State> make_draft_menu(Project project, FiscalYear year);

// Tells the state the TUI goes back to what was done, as lines for its view,
// and the statement file read
using StatementReport = std::function<void(pyroceph::tui::View, std::filesystem::path)>;

// Läs kontoutdrag: account statements read into drafts (beslut 2026-11-17).
// The user types the path of the statement, starting in the project folder;
// Tab completes folders and all files (no filter on what a statement may be).
// Enter on anything but a file (nothing typed, a folder, a path that does not
// exist), or on a file cratchit cannot read as a statement (see
// read_statement_file), is refused with the reason, and the path is kept.
// The path starts in start_folder when given, else the project folder; coming
// back, in the folder of the file chosen last.
//
// A statement read leads to "Konto": what the file holds (encoding, entries,
// saldo, lines that are not entries) and the account cratchit suggests (see
// guess_statement_account), typed in for the user. Enter takes a BAS account
// in the books' chart of accounts and leads to "Bekräfta": what reading it
// into drafts of year would do (see plan_statement_drafts), with the new
// entries, J Ja and N Nej, or just N Tillbaka when nothing is new. J adds the
// drafts (add_statement_drafts), gives report what was done and goes back to
// origin (State::back_to); N goes back to choose a file. A failure leads to
// the error state.
std::unique_ptr<pyroceph::tui::State> make_read_statement_menu(Project project, FiscalYear year,
                                                               pyroceph::tui::State const* origin, StatementReport report,
                                                               std::optional<std::filesystem::path> start_folder = {});

// Bokför for the draft at index of count in year, the one seen (as from the
// draft's view): the series typed, Enter books it (book_draft). Booked, the TUI
// goes back to back, and report gets what was done (e.g. "cratchit bokförde #2
// som M3: Momsrapport 2026-07-01 - 2026-09-30."). - goes back without booking.
std::unique_ptr<pyroceph::tui::State> make_book_draft_state(Project project, FiscalYear year, std::size_t index,
                                                            std::size_t count, books::Draft draft,
                                                            pyroceph::tui::State const* back,
                                                            std::function<void(std::string)> report);

} // namespace cratchit::ui
