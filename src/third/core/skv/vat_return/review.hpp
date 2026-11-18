#pragma once

#include "core/books/books.hpp"
#include "core/skv/vat_return/box.hpp"
#include "core/skv/vat_return/document.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Reviewing a VAT return against SKV's descriptions of the boxes (see box_texts): the boxes to look at, and each
// box with its postings next to its description
namespace cratchit::skv::vat_return {

// A box to review: its section and main title on the form, and how many postings the period has in it
struct ReviewBox {
  Box box;
  std::string section;
  std::string title;
  std::size_t postings = 0; // counted postings (not removed ones) on its accounts

  bool operator==(ReviewBox const&) const = default;
};

// Every box of the form, in the form's order, box 49 aside (it is added up, not posted to), each with its main
// title (box 48: Ingående moms att dra av) and how many postings it has; boxes without postings too, so a box that
// is missing shows.
std::vector<ReviewBox> review_boxes(Document const& document);

// The lines of one box for review: its number, main title (and any other title the form gives it, as 48) and
// section; its amount on the form or that it has no
// postings; its accounts with their sums and names and the vouchers behind them with their texts; the accounts
// that report in it (see accounts_of) without postings, each with its name in the books' chart of accounts; then
// SKV's description (each of its titles' descriptions, apart by an empty line), wrapped to width, and an empty line
std::vector<std::string> review_of(books::Books const& books, Document const& document, Box const& box,
                                   std::size_t width = 80);

// The text cut to at most max characters (code points), ending with ... when cut
std::string cut_text(std::string_view text, std::size_t max);

// Text in lines of at most width characters (code points), broken between words; LF starts a new line and an
// empty line stays. A word longer than width gets a line of its own.
std::vector<std::string> wrap_text(std::string_view text, std::size_t width);

} // namespace cratchit::skv::vat_return
