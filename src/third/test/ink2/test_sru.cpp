#include "core/sru.hpp"

#include <gtest/gtest.h>

using cratchit::parse_sru;

TEST(Sru, Forms) {
  auto const forms = parse_sru("#BLANKETT  INK2R-2025P1\r\n"
                               "#IDENTITET  556782-8172 20250830 120000\r\n"
                               "#SYSTEMINFO u.a.\r\n"
                               "#UPPGIFT 7011 20240501\r\n"
                               "#UPPGIFT 7513 15530 \r\n"
                               "#BLANKETTSLUT\r\n"
                               "#BLANKETT INK2S-2025P1\r\n"
                               "#UPPGIFT 8041 X\r\n"
                               "#BLANKETTSLUT\r\n"
                               "#FIL_SLUT");
  ASSERT_TRUE(forms) << forms.error();
  ASSERT_EQ(forms->size(), 2u);
  EXPECT_EQ((*forms)[0].name, "INK2R-2025P1");
  EXPECT_EQ((*forms)[0].kind(), "INK2R");
  EXPECT_EQ((*forms)[0].identity, "556782-8172 20250830 120000");
  EXPECT_EQ((*forms)[0].fields, (std::map<std::string, std::string>{{"7011", "20240501"}, {"7513", "15530"}}));
  EXPECT_EQ((*forms)[1].kind(), "INK2S");
  EXPECT_EQ((*forms)[1].fields.at("8041"), "X");
}

TEST(Sru, Errors) {
  EXPECT_EQ(parse_sru("#UPPGIFT 7011 20240501\n").error(), "SRU line 1: #UPPGIFT outside a form");
  EXPECT_EQ(parse_sru("#BLANKETT A\n#UPPGIFT 1 2\n#UPPGIFT 1 3\n").error(), "SRU line 3: field 1 given twice");
  EXPECT_EQ(parse_sru("#BLANKETT A\n").error(), "SRU: the form A is not ended by #BLANKETTSLUT");
}
