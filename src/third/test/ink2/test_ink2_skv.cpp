#include "core/ink2/mapping.hpp"
#include "core/ink2/skv.hpp"

#include <gtest/gtest.h>

#include <algorithm>

using cratchit::ink2::fields_of;
using cratchit::ink2::parse_skv_fields;
using cratchit::ink2::SkvField;

namespace {

std::vector<SkvField> const& built_in() {
  static auto const fields = *parse_skv_fields(cratchit::ink2::skv_fields_text());
  return fields;
}

SkvField const& field(std::string_view sru) {
  return *std::ranges::find(built_in(), sru, &SkvField::sru);
}

} // namespace

TEST(Ink2Skv, BuiltInHasTheThreeForms) {
  ASSERT_TRUE(parse_skv_fields(cratchit::ink2::skv_fields_text()));
  EXPECT_EQ(fields_of(built_in(), "INK2").size(), 19u);
  EXPECT_EQ(fields_of(built_in(), "INK2R").size(), 84u); // 2.1-2.50, 3.1-3.27 and the 7 minus fields
  EXPECT_EQ(fields_of(built_in(), "INK2S").size(), 48u);
}

TEST(Ink2Skv, Fields) {
  EXPECT_EQ(field("7653").form, "INK2S");
  EXPECT_EQ(field("7653").ink2, "4.3c");
  EXPECT_EQ(field("7653").sign, '+');
  EXPECT_EQ(field("7763").ink2, "4.14a");
  EXPECT_EQ(field("7763").sign, '-');
  EXPECT_EQ(field("7770").text, "Underskott (flyttas till p. 1.2 på sid. 1)");
  EXPECT_EQ(field("7114").form, "INK2");
  EXPECT_EQ(field("7114").sign, '*');
  EXPECT_EQ(field("8041").type, SkvField::Type::mark);
  EXPECT_EQ(field("8041").ink2, "");
  EXPECT_EQ(field("7510").ink2, "3.2");
}

TEST(Ink2Skv, SignsOfINK2SFollowTheirCodes) {
  // 76xx add to överskott and 77xx take from it, which is what the mapping's signs rely on
  for (auto const& skv : fields_of(built_in(), "INK2S")) {
    if (skv.sru.starts_with("76")) {
      EXPECT_EQ(skv.sign, '+') << skv.sru;
    } else if (skv.sru.starts_with("77")) {
      EXPECT_EQ(skv.sign, '-') << skv.sru;
    }
  }
}

TEST(Ink2Skv, TheMappingHasSkvsCodesAndNumbers) {
  auto const lines = cratchit::ink2::parse_mapping(cratchit::ink2::mapping_text());
  ASSERT_TRUE(lines);
  std::size_t ink2r_codes = 0;
  for (auto const& line : *lines) {
    for (auto const& mapped : line.fields) {
      auto const found = std::ranges::find(built_in(), mapped.sru, &SkvField::sru);
      ASSERT_NE(found, built_in().end()) << mapped.sru;
      EXPECT_EQ(found->form, line.form) << mapped.sru;
      EXPECT_EQ(found->ink2, line.ink2) << mapped.sru;
      ink2r_codes += line.form == "INK2R";
    }
  }
  EXPECT_EQ(ink2r_codes, fields_of(built_in(), "INK2R").size());
}

TEST(Ink2Skv, Errors) {
  EXPECT_EQ(parse_skv_fields("form;sru\n").error(),
            "The SKV fields do not start with the header form;sru;ink2;text;sign;type");
  auto const header = std::string("form;sru;ink2;text;sign;type\n");
  EXPECT_EQ(parse_skv_fields(header + "INK3;7104;1.1;x;*;amount\n").error(), "SKV fields row 2: unknown form \"INK3\"");
  EXPECT_EQ(parse_skv_fields(header + "INK2;71x4;1.1;x;*;amount\n").error(),
            "SKV fields row 2: \"71x4\" is not an SRU code");
  EXPECT_EQ(parse_skv_fields(header + "INK2;7104;1.1;x;*;amount\nINK2;7104;1.1;x;*;amount\n").error(),
            "SKV fields row 3: SRU code 7104 given twice");
  EXPECT_EQ(parse_skv_fields(header + "INK2;7104;1.1;x;/;amount\n").error(),
            "SKV fields row 2: sign \"/\" is not +, - or *");
  EXPECT_EQ(parse_skv_fields(header + "INK2;90;;x;+;mark\n").error(), "SKV fields row 2: a mark has no sign");
  EXPECT_EQ(parse_skv_fields(header + "INK2;90;;x;;date\n").error(),
            "SKV fields row 2: type \"date\" is not amount or mark");
}
