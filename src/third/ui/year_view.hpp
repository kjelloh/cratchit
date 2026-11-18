#pragma once

#include "core/books/books.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit::ui {

// The width of the text of a draft of stage samma, in code points (see draft_text)
inline constexpr std::size_t same_text_width = 40;

// The rows a fiscal year shows at most, for now
inline constexpr std::size_t year_view_rows = 10;

// What a fiscal year's view shows of its current zoom. For now the zoom is
// always the whole year.
struct YearView {
  // What the zoom holds, e.g. "Hela året: 37 verifikationer, 1 utkast" or
  // "Hela året: inget bokfört"
  std::string summary;
  // The zoom's vouchers and drafts, newest first (by date; on the same date a
  // draft before the vouchers, and later ones before earlier ones), at most
  // max_rows of them, then "… 27 äldre" when older ones are left out. A
  // voucher shows its series and number, a draft its stage (see stage_text).
  // Not options: just a view.
  std::vector<std::string> rows;
};

// The view of year; nullptr is a year with nothing in the books yet
YearView year_view(books::FiscalYear const* year, std::size_t max_rows = year_view_rows);

// The year's vouchers, one line each as in year_view, newest first (by date;
// on the same date later ones in the books before earlier ones). None for nullptr.
std::vector<std::string> voucher_rows(books::FiscalYear const* year);

// A draft's id as the user sees it: its place among the year's count drafts,
// newest first, from 1, after '#', e.g. "#3" for index 7 of 10 (index is in date
// order, oldest first). Booking from the oldest keeps the others' ids; browsing
// from the oldest the ids count down (beslut 2026-11-18). Not stable: it changes
// when a draft of a later date is added or a newer draft leaves the list.
std::string draft_id(std::size_t index, std::size_t count);

// The index (date order) of the draft with id number (as draft_id) among count
// drafts; none when there is no such draft
std::optional<std::size_t> draft_index(std::size_t number, std::size_t count);

// How many drafts the year starting at start has in the books in folder (0 when
// they cannot be read), for the id of a draft just added
std::size_t draft_count(std::filesystem::path const& folder, Date start);

// What a draft's row tells about it: its heading, else its first posting's
// account and text (e.g. a statement entry), then its stage in parentheses,
// e.g. "1920  AVGIFTER NORDEA (kontohändelse)". A draft of stage samma shows
// the raw texts of its parts instead, the heading first, joined by " : " and
// without accounts, in at most same_text_width code points: a part shorter than
// its share stays whole, the longer ones share what is left and end in "..",
// e.g. "Anthropic Claude : Kortköp 260910 ANTH.. (samma)".
std::string draft_text(books::Draft const& draft);

// The year's drafts, one line each as voucher_rows, newest first (the drafts
// are kept oldest first, ties in the order noted), each with its draft_id. A
// draft's amount is its event's, with its sign; a draft without one shows its
// only posting's amount with its sign (e.g. -2292.00 out of the account), or
// what its postings move when it has several. None for nullptr.
std::vector<std::string> draft_rows(books::FiscalYear const* year);

// The text of a draft of stage samma as draft_text shows it, without the stage
// (e.g. "Anthropic Claude : Kortköp 260910 ANTH.."); empty for any other stage
std::string same_text(books::Draft const& draft);

// One row of draft_rows: the draft at index of count
std::string draft_row(std::size_t index, std::size_t count, books::Draft const& draft);

// One row of voucher_rows
std::string voucher_row(books::Voucher const& voucher);

// The stage of a draft for the user: "händelse", "kontohändelse", "samma", "obalanserat" or "klart"
std::string_view stage_text(books::DraftStage stage);

} // namespace cratchit::ui
