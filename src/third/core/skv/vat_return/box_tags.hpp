#pragma once

#include "core/skv/vat_return/box.hpp"

#include <expected>
#include <string>
#include <string_view>
#include <vector>

// The eSKD tags of the boxes, as built into the program from
// resources/momsrapport/momsrapport_box_to_eskd.csv. Behind eskd::document_of; here for the tests.
namespace cratchit::skv::vat_return {

// A box and the XML element the eSKD file gives it in, e.g. 10 and MomsUtgHog
struct BoxTag {
  Box box;
  std::string tag;

  bool operator==(BoxTag const&) const = default;
};

// The built-in text: a heading line `box;tag`, then one line per box, `box;tag`,
// in the order the tags have in the eSKD file
std::string_view box_tags_text();

// The boxes and tags of such text, in its order. A UTF-8 byte order mark, a
// first line `box;tag`, CR before LF and blank lines are skipped. Error text
// names the line whose box is not two digits, whose tag is not ASCII letters
// and digits, or whose box or tag is listed before.
std::expected<std::vector<BoxTag>, std::string> box_tags(std::string_view text);

} // namespace cratchit::skv::vat_return
