#pragma once

#include "core/skv/vat_return/box.hpp"

#include <expected>
#include <map>
#include <string>
#include <string_view>

// The boxes accounts report in, as built into the program from
// resources/momsrapport/bas_to_momsrapport.csv. Behind box_of; here for the tests.
namespace cratchit::skv::vat_return {

// The built-in text: a heading line `in;vat;out;hint`, then one line per account,
// `in;box;out;code`. The account is in `in` (e.g. `2611;10;;U1`) when its postings
// add up into the box, or in `out` when it is a source of the box (e.g. `;49;2650;R2`)
std::string_view account_boxes_text();

// The box of each account in such text. A UTF-8 byte order mark, a first line
// starting with `in;`, CR before LF, blank lines and lines with only an `out`
// account are skipped; the code is not read. Error text names the line
// whose account is not four digits, whose box is not two digits, or whose
// account is listed before.
std::expected<std::map<std::string, Box>, std::string> account_boxes(std::string_view text);

} // namespace cratchit::skv::vat_return
