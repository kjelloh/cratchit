#pragma once

#include "core/books/books.hpp"
#include "core/date.hpp"

#include <cstddef>
#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

// Drafts that may be the same financial event as another draft, or as a
// voucher already in the books, as take 3 (60cae9fe) found matches among its
// waiting events (beslut 2026-11-17). The user decides; cratchit only lists
// the candidates and makes two drafts one.
namespace cratchit {

// How many days apart two events may be and still be the same event: a weekend
// and a bank day on each side (Thursday to Monday). Calendar days, no holidays.
inline constexpr int same_event_window_days = 4;

// Two that may be the same event: the same amount without sign, at most the window
// apart. Two drafts only when they fit (see fits): at most one has an event, and
// when both have postings these balance together on different accounts.
struct SameEvent {
  enum class Kind {
    drafts, // two drafts
    booked, // a draft and a voucher in the books
  };

  Kind kind = Kind::drafts;
  std::size_t draft = 0; // the draft, an index into the year's drafts (the older of two)
  std::size_t other = 0; // the other draft (drafts), or the voucher (booked), an index into the year's
  int days = 0;          // the days between their dates
  std::vector<std::string> words; // the words their texts share (see shared_words)
  // drafts: always (pairs that do not fit are not listed); booked: the draft has
  // postings, all in the voucher
  bool fits = false;

  bool operator==(SameEvent const&) const = default;
};

// What a draft amounts to without sign: its event's amount, else what its postings move
Amount amount_of(books::Draft const& draft);

// The text a draft is compared by: its heading, else the texts of its postings
std::string compared_text(books::Draft const& draft);

// The words two texts share, in alphabetical order: letters and digits (and any
// non-ASCII character), case folded, without numbers and one-letter words
std::vector<std::string> shared_words(std::string_view a, std::string_view b);

// The possible pairs in year, the likely ones first: those that fit, then more
// shared words, then fewer days apart, then the older draft. A pair of drafts is
// listed once; a draft may be in several pairs. An amount of zero pairs with nothing.
std::vector<SameEvent> find_same_events(books::FiscalYear const& year, int window_days = same_event_window_days);

// The postings of draft not booked by voucher: none of the voucher's postings
// (not removed) has its account, date and amount. Each voucher posting books one.
books::Postings unbooked(books::Draft const& draft, books::Voucher const& voucher);

// One draft of two: event, the earlier noted day, and the postings of both (a
// posting of b from the same statement entry as one of a's left out)
books::Draft merged(books::Draft const& a, books::Draft const& b, books::Event event);

// Makes the drafts at index a and b (a != b) of the fiscal year starting on
// year_start one (see merged, with event) and writes the books. Only when the
// drafts there are still the expected ones (the ones the user saw), as drafts
// are numbered by their place and may have changed. Gives the index of the one
// draft. Error text when the books cannot be read or written, the year is not
// in them, or a draft is not there any more. Does not log.
std::expected<std::size_t, std::string> merge_drafts(std::filesystem::path const& project_folder, Date year_start,
                                                     std::size_t a, books::Draft const& expected_a, std::size_t b,
                                                     books::Draft const& expected_b, books::Event const& event);

// What a draft is made of: see books::parts_of
using books::DraftParts;
using books::parts_of;

// What 9 does to a draft (beslut 2026-11-18): a draft without postings from
// statement entries can always be forgotten (nothing is lost that cannot be made
// again); one with them is forgotten when lone, split when made of the same
// (Inte samma), and neither when more than the same, so that the entries stay.
enum class DraftRemoval { forget, split, none };

DraftRemoval removal_of(books::Draft const& draft);

// The lone drafts a draft is made of: its event, without postings (when it has
// a heading or an amount), then each posting (not removed) as a draft of its
// own, dated as the posting (else the event), with no heading and no amount.
// Each is noted when the draft was.
std::vector<books::Draft> split(books::Draft const& draft);

// Splits the draft at index of the fiscal year starting on year_start into its
// lone drafts (see split) and writes the books. Only for a draft of parts same,
// and only when the draft there is still the expected one (the one the user
// saw). Gives how many drafts it became. Error text when the books cannot be
// read or written, the year is not in them, the draft is not there any more or
// is not made of the same. Does not log.
std::expected<std::size_t, std::string> split_draft(std::filesystem::path const& project_folder, Date year_start,
                                                    std::size_t index, books::Draft const& expected);

} // namespace cratchit
