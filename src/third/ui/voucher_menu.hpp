#pragma once

#include "core/books/books.hpp"
#include "pyroceph/tui/state.hpp"

#include <cstddef>
#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace cratchit::ui {

// The postings shown at a time in "Konteringar", with keys 0–9
inline constexpr std::size_t postings_page_rows = 10;

// A booked voucher, named by its id in the path (e.g. "A12"; see voucher_id).
// The view has three parts (beslut.md):
// * how it is booked: its series with series_name (just the series when the
//   name is empty), the day it was registered and the signature (when known);
// * "Händelse": its date, heading and amount;
// * "Konteringar": its postings, one per line: account, account name (from
//   accounts), amount, the posting's own text and date when it has them, and
//   "struken" or "tillagd" for a struck or added posting.
// Options 0 "Händelse" and 1 "Konteringar" lead to those parts.
// * The event of a booked voucher is locked (beslut.md): its options (date,
//   heading, amount) lead to "Låst (<option>)", which says so and tells what
//   can be done instead, depending on whether the voucher is the last in its
//   series (last_in_series, the id of the last one in the fiscal year).
// * The options of a posting (account, description, amount) lead to Inte än
//   (make_design_insufficiency_state) for now.
// To grow into the voucher editor.
// 2 Ändringsverifikation (beslut 2026-11-18) calls reverse, which makes the
// draft that reverses the voucher (add_reversal_draft) and gives what to tell
// the user; that is shown below the voucher, and the user stays. An error text
// is shown the same way. The option is there only when reverse is given.
using ReverseVoucher = std::function<std::expected<pyroceph::tui::View, std::string>()>;
// 3 Utskrift (beslut 2026-11-18) calls print, which gives the state that shows
// the voucher as a whole document (see make_voucher_printout); an error text
// leads to Fel. The option is there only when print is given.
using PrintVoucher = std::function<std::expected<std::unique_ptr<pyroceph::tui::State>, std::string>()>;
std::unique_ptr<pyroceph::tui::State> make_voucher_menu(books::Voucher voucher, std::vector<books::Account> accounts,
                                                        std::string series_name, std::string last_in_series,
                                                        ReverseVoucher reverse = {}, PrintVoucher print = {});

// A draft, named by its id in the path ("#3", see draft_id), shown as a voucher
// is: the heading "Utkast #3 i 2026/27 (<stage>)", the day it was noted, then
// "Händelse" (a heading or amount it lacks shows empty) and "Konteringar".
//
// 9 is what removal_of gives (beslut 2026-11-17 and -18: 9 is the option that
// removes, kept apart from the others):
// * forget (no statement entries, or lone): 9 Glöm leads to "Glöm": it shows
//   summary, says the draft cannot be had back, and offers 9 Glöm and N Nej.
//   9 again calls forget; done, the TUI goes back to origin (State::back_to,
//   Utkast); N goes back to the draft.
// * split (statement entries, the same): 9 Inte samma calls split at once (it
//   can be undone in Samma?); done, back to origin.
// * none (statement entries, more than the same): no 9.
// A failure of forget or split leads to the error state with its text.
//
// 0 Händelse and 1 Konteringar change the draft (beslut 2026-11-18: one editor
// for drafts, Kontera): they lead to what edit_event (with the view to go back
// to) and edit_postings give. 2 Bokför, only for a draft that is ready, leads
// to what book gives. Coming back, the view shows the draft as now
// gives it (it may have a new id, as its date may have changed).
using ForgetDraft = std::function<std::expected<void, std::string>()>;

// A draft as it is now, after it was changed: the draft, its id and its summary line
struct DraftNow {
  books::Draft draft;
  std::string id;
  std::string summary;
};

// What can be done to a draft from its view
struct DraftActions {
  ForgetDraft forget;
  ForgetDraft split;
  std::function<std::unique_ptr<pyroceph::tui::State>(pyroceph::tui::State const* back)> edit_event;
  std::function<std::unique_ptr<pyroceph::tui::State>()> edit_postings;
  // Bokför, for a draft that is ready (2 Bokför), with the view
  std::function<std::unique_ptr<pyroceph::tui::State>(pyroceph::tui::State const* view)> book;
  // Mall (3 Använd mall), with the view to go back to once a template is used
  std::function<std::unique_ptr<pyroceph::tui::State>(pyroceph::tui::State const* view)> use_template;
  std::function<std::optional<DraftNow>()> now;
};

std::unique_ptr<pyroceph::tui::State> make_draft_view(books::Draft const& draft, std::string id,
                                                      std::string year_label, std::vector<books::Account> accounts,
                                                      std::string summary, pyroceph::tui::State const* origin,
                                                      DraftActions actions);

// The lines of a draft's view (as make_draft_view shows it, without options),
// with id in its heading, e.g. "Utkast #1 av 18 i 2026/27 (kontohändelse)"
pyroceph::tui::View draft_view_lines(books::Draft const& draft, std::string const& id, std::string year_label,
                                     std::vector<books::Account> accounts);

// What a draft lacks to be booked, each with where to fix it, after the line
// "Innan utkastet kan bokföras:", e.g. "  Rubrik saknas: tryck 0 och sedan 1.";
// empty when it is ready. Shown last in the draft's view and in Fullborda.
pyroceph::tui::View missing_to_book(books::Draft const& draft);

// "Glöm" for the draft id (e.g. "#3"), as from a draft's view: it shows summary
// (one line about the draft), says the draft cannot be had back, and offers
// 9 Glöm and N Nej. 9 calls forget; done, the TUI goes back to forgotten_to;
// N goes back to declined_to. A failure of forget leads to the error state.
std::unique_ptr<pyroceph::tui::State> make_forget_draft_state(std::string id, std::string summary, ForgetDraft forget,
                                                              pyroceph::tui::State const* forgotten_to,
                                                              pyroceph::tui::State const* declined_to);

} // namespace cratchit::ui
