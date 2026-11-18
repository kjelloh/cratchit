#include "core/ink2/accounts.hpp"

#include <gtest/gtest.h>

#include <string>

using cratchit::ink2::Condition;
using cratchit::ink2::parse_account_set;

namespace {

// The accounts 1000-9999 in a set, written for comparing; ranges as first-last
std::string members(std::string_view text) {
  auto const set = parse_account_set(text);
  EXPECT_TRUE(set.has_value()) << (set ? "" : set.error());
  std::string out;
  int first = 0;
  int last = -1;
  auto const flush = [&] {
    if (last >= first) {
      out += (out.empty() ? "" : " ") + (first == last ? std::to_string(first) : std::format("{}-{}", first, last));
    }
  };
  for (int number = 1000; set && number <= 9999; ++number) {
    if (set->contains(std::to_string(number))) {
      if (number != last + 1) {
        flush();
        first = number;
      }
      last = number;
    }
  }
  flush();
  return out;
}

} // namespace

TEST(Ink2AccountSet, SingleAccount) {
  EXPECT_EQ(members("1088"), "1088");
}

TEST(Ink2AccountSet, Patterns) {
  EXPECT_EQ(members("19xx"), "1900-1999");
  EXPECT_EQ(members("112x"), "1120-1129");
}

TEST(Ink2AccountSet, RangesWithHyphenOrEnDash) {
  EXPECT_EQ(members("211x-213x"), "2110-2139");
  EXPECT_EQ(members("151x–155x, 158x"), "1510-1559 1580-1589");
  EXPECT_EQ(members("40xx-47xx, 4910-4929"), "4000-4799 4910-4929");
}

TEST(Ink2AccountSet, ExclusionOfTheWholeList) {
  EXPECT_EQ(members("10xx (exkl. 1088)"), "1000-1087 1089-1099");
  EXPECT_EQ(members("133x (exkl. 1336 och 1337)"), "1330-1335 1338-1339");
  EXPECT_EQ(members("156x-157x, 166x,167x (exkl. 1573, 1673)"), "1560-1572 1574-1579 1660-1672 1674-1679");
  EXPECT_EQ(members("246x-247x, 286x-287x (exkl. 2473, 2873)"), "2460-2472 2474-2479 2860-2872 2874-2879");
}

TEST(Ink2AccountSet, ExclusionThenMore) {
  // The exclusion applies to the terms before it only: 2473 and 2873 are in
  EXPECT_EQ(members("249x (exkl. 2492), 26xx-27xx, 28xx (exkl. 286x-287x), 2473, 2873"),
            "2473 2490-2491 2493-2499 2600-2859 2873 2880-2899");
}

TEST(Ink2AccountSet, Conditions) {
  auto const set = parse_account_set("49xx (exkl. 4910-4929, 496x, 498x) (Om netto +)");
  ASSERT_TRUE(set);
  ASSERT_EQ(set->groups.size(), 1u);
  EXPECT_EQ(set->groups[0].condition, Condition::credit);
  EXPECT_TRUE(set->contains("4900"));
  EXPECT_FALSE(set->contains("4961"));
  auto const debit = parse_account_set("49xx (exkl. 4910-4929, 496x, 498x) (Om  netto -)");
  ASSERT_TRUE(debit);
  EXPECT_EQ(debit->groups[0].condition, Condition::debit);
}

TEST(Ink2AccountSet, ConditionOfPartOfTheSet) {
  auto const set = parse_account_set("8810 (Om netto +), 8819");
  ASSERT_TRUE(set);
  ASSERT_EQ(set->groups.size(), 2u);
  EXPECT_EQ(set->groups[0].condition, Condition::credit);
  EXPECT_TRUE(set->groups[0].contains("8810"));
  EXPECT_EQ(set->groups[1].condition, Condition::always);
  EXPECT_TRUE(set->groups[1].contains("8819"));
  auto const both = parse_account_set("886x-889x, 884x (Om netto -)");
  ASSERT_TRUE(both);
  ASSERT_EQ(both->groups.size(), 1u);
  EXPECT_EQ(both->groups[0].condition, Condition::debit);
  EXPECT_EQ(members("886x-889x, 884x (Om netto -)"), "8840-8849 8860-8899");
}

TEST(Ink2AccountSet, YearsResult) {
  auto const profit = parse_account_set("+ 899x");
  ASSERT_TRUE(profit);
  EXPECT_EQ(profit->groups[0].condition, Condition::profit);
  auto const loss = parse_account_set("– 899x");
  ASSERT_TRUE(loss);
  EXPECT_EQ(loss->groups[0].condition, Condition::loss);
  EXPECT_EQ(members("– 899x"), "8990-8999");
}

TEST(Ink2AccountSet, Unreadable) {
  for (std::string_view const text : {"", "12", "12345", "1x2x", "10xx (exkl.)", "10xx (Om netto)", "10xx,",
                                      "10xx eller 11xx", "11xx-10xx", "(Om netto +)", "+ 10xx (Om netto +)"}) {
    EXPECT_FALSE(parse_account_set(text).has_value()) << text;
  }
  auto const error = parse_account_set("10xx; 11xx");
  ASSERT_FALSE(error);
  EXPECT_EQ(error.error(), "Unreadable BAS accounts \"10xx; 11xx\" at \"; 11xx\"");
}

TEST(Ink2AccountSet, OnlyFourDigitAccounts) {
  auto const set = parse_account_set("19xx");
  ASSERT_TRUE(set);
  EXPECT_TRUE(set->contains("1930"));
  EXPECT_FALSE(set->contains("193"));
  EXPECT_FALSE(set->contains("19300"));
}
