#pragma once

#include "core/amount.hpp"
#include "core/skv/vat_return/box.hpp"

#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The boxes' texts on SKV's VAT return form, as built into the program from
// resources/momsrapport/momsrapport.csv: the section of the form a box is in and its title
namespace cratchit::skv::vat_return {

// A box with its section, title and SKV's description, e.g. 10, Momspliktig försäljning eller uttag,
// Utgående moms 25%, "Utgående moms 25%\nFyll i den utgående momsen ..."
struct BoxText {
  Box box;
  std::string section;
  std::string title;
  std::string description; // lines apart by LF

  bool operator==(BoxText const&) const = default;
};

// The built-in text: a heading line `Box;Section;Section;Description`, then one row per box title,
// `box;section;title;description` (the description quoted, over several lines), in the form's order
std::string_view box_texts_text();

// The boxes of such text, in its order (a box may have two titles, as 48). A UTF-8 byte order mark, the heading
// and rows without a box are skipped; CR is dropped from the description. Error text names the line whose box is
// not two digits or that has no section or title.
std::expected<std::vector<BoxText>, std::string> box_texts(std::string_view text);

// The section and title of a box from the built-in text; box 48 with a negative amount (on the form) takes its
// second title, Tidigare avdragen moms att betala tillbaka. Nothing for a box the text lacks.
std::optional<BoxText> text_of(Box const& box, Amount amount = 0);

// The built-in boxes, in the form's order (see box_texts); none when the built-in text cannot be read
std::vector<BoxText> const& built_in_box_texts();

// Where a box comes on the form (its first row in the built-in text, from 0); boxes the text lacks come last
std::size_t form_position(Box const& box);

} // namespace cratchit::skv::vat_return
