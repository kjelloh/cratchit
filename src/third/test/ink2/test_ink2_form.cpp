#include "core/ink2/form.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <iterator>

using cratchit::Balances;
using cratchit::ink2::build_form;
using cratchit::ink2::Form;
using cratchit::ink2::Line;

namespace {

std::vector<Line> const& mapping() {
  static auto const lines = *cratchit::ink2::parse_mapping(cratchit::ink2::mapping_text());
  return lines;
}

// The fields used, as SRU code=kronor
std::string used(Form const& form) {
  std::string out;
  for (auto const& [sru, field] : form.fields) {
    if (field.used) {
      out += std::format("{}{}={}", out.empty() ? "" : " ", sru, field.kronor);
    }
  }
  return out;
}

// The notes on the books' #SRU
std::vector<std::string> sru_notes(Form const& form) {
  std::vector<std::string> out;
  std::ranges::copy_if(form.notes, std::back_inserter(out), [](std::string const& note) { return note.starts_with("#SRU"); });
  return out;
}

} // namespace

TEST(Ink2Form, SignsOfTheFields) {
  // A year with a profit of 100 kr: sales 1000, costs 900; bank 1100, debt 200, equity 800 + 100
  Balances const balances{{"1930", 110000}, {"2440", -20000}, {"2081", -80000}, {"2099", -10000},
                          {"3001", -100000}, {"5410", 90000}, {"8999", 10000}};
  auto const form = build_form(mapping(), balances);
  EXPECT_TRUE(form.errors.empty());
  EXPECT_EQ(used(form), "7281=1100 7301=800 7302=100 7365=200 7410=1000 7450=100 7513=900");
  EXPECT_TRUE(form.notes.empty());
}

TEST(Ink2Form, LossGoesTo7550) {
  Balances const balances{{"1930", 50000}, {"2081", -80000}, {"2099", 30000}, {"6110", 30000}, {"8999", -30000}};
  auto const form = build_form(mapping(), balances);
  EXPECT_EQ(used(form), "7281=500 7301=800 7302=-300 7513=300 7550=300");
  EXPECT_FALSE(form.fields.at("7450").used);
}

TEST(Ink2Form, NegativeFieldsAreShownNegative) {
  // A debit on leverantörsskulder and öresavrundning taking from nettoomsättning, as in the filed INK2R
  Balances const balances{{"1930", 89625}, {"2440", 89625}, {"2081", -179250}, {"3740", 362}, {"8999", -362}};
  auto const form = build_form(mapping(), balances);
  EXPECT_EQ(form.fields.at("7365").kronor, -896);
  EXPECT_EQ(form.fields.at("7410").kronor, -4);
}

TEST(Ink2Form, ConditionByTheNetOfTheGroup) {
  // Återföring of periodiseringsfond (a credit on 8810) is 3.21; avsättning (a debit) 3.22
  auto const back = build_form(mapping(), {{"8810", -5000}, {"8819", -1000}});
  EXPECT_EQ(used(back), "7420=60");
  auto const set_aside = build_form(mapping(), {{"8810", 5000}, {"8811", 1000}});
  EXPECT_EQ(used(set_aside), "7525=60");
  // Lagerförändring: the net of 49xx (but 4910-4929, 496x, 498x) decides
  auto const stock = build_form(mapping(), {{"4900", -3000}, {"4990", 1000}});
  EXPECT_EQ(used(stock), "7411=20");
  auto const less_stock = build_form(mapping(), {{"4900", 3000}, {"4990", -1000}});
  EXPECT_EQ(used(less_stock), "7510=20");
}

TEST(Ink2Form, AccountInNoFieldOrTwo) {
  auto const form = build_form(mapping(), {{"8710", -19550}, {"4010", 100}, {"9021", 1000}, {"9099", -500}});
  EXPECT_EQ(form.errors, (std::vector<std::string>{
                             "Account 4010 (1.00) is in more than one field: 7511, 7512, and it has no #SRU to choose",
                             "Account 8710 has an amount -195.50 but is in no field of the mapping",
                         }));
  EXPECT_EQ(form.notes.front(), "Left out, as not BAS accounts of class 1-8: 9021, 9099 (together 5.00)");
}

TEST(Ink2Form, SruChoosesBetweenFields) {
  auto const form = build_form(mapping(), {{"4010", 10000}}, {{"4010", "7512"}});
  EXPECT_TRUE(form.errors.empty());
  EXPECT_EQ(used(form), "7512=100");
  EXPECT_EQ(sru_notes(form), (std::vector<std::string>{"#SRU: account 4010 is in 7511, 7512 in the mapping; its #SRU chooses 7512"}));
  auto const other = build_form(mapping(), {{"4010", 10000}}, {{"4010", "7513"}});
  EXPECT_EQ(other.errors,
            (std::vector<std::string>{"Account 4010 (100.00) is in more than one field: 7511, 7512, and its #SRU 7513 is "
                                      "none of them"}));
  EXPECT_EQ(used(other), "");
}

TEST(Ink2Form, SruIsTheFallback) {
  auto const form = build_form(mapping(), {{"8710", -19550}}, {{"8710", "7413"}});
  EXPECT_TRUE(form.errors.empty());
  EXPECT_EQ(used(form), "7413=196");
  EXPECT_EQ(sru_notes(form), (std::vector<std::string>{"#SRU: account 8710 (-195.50) is in no field of the mapping; put in "
                                                  "7413 by its #SRU"}));
  // An SRU code not on INK2R, as Fortnox's 7418 for 8710
  auto const old = build_form(mapping(), {{"8710", -19550}}, {{"8710", "7418"}});
  EXPECT_EQ(old.errors, (std::vector<std::string>{"Account 8710 has an amount -195.50 but is in no field of the "
                                                  "mapping, nor is its #SRU 7418"}));
}

TEST(Ink2Form, SruDisagreeingIsANote) {
  auto const form = build_form(mapping(), {{"2650", 42800}, {"1930", -42800}}, {{"2650", "7261"}, {"1930", "7281"}});
  EXPECT_EQ(used(form), "7281=-428 7369=-428");
  EXPECT_EQ(sru_notes(form), (std::vector<std::string>{"#SRU: account 2650 has #SRU 7261 but the mapping puts it in 7369 (used)"}));
}

TEST(Ink2Form, SruOfTheOtherFieldOfAPairAgrees) {
  // Fortnox gives 8999 the SRU code 7450 also in a year with a loss
  auto const form = build_form(mapping(), {{"8999", -3000}, {"8810", -100}}, {{"8999", "7450"}, {"8810", "7525"}});
  EXPECT_EQ(used(form), "7420=1 7550=30");
  EXPECT_TRUE(sru_notes(form).empty());
}

TEST(Ink2Form, AccountsWithoutSru) {
  auto const form = build_form(mapping(), {{"1226", 100}, {"1227", 100}, {"1229", -100}}, {{"1229", "7215"}});
  EXPECT_EQ(sru_notes(form), (std::vector<std::string>{"#SRU: no #SRU in the books for 1226, 1227; the mapping decides"}));
  // Books without any #SRU get no such note
  EXPECT_TRUE(sru_notes(build_form(mapping(), {{"1226", 100}, {"1229", -100}})).empty());
}

TEST(Ink2Form, ZeroAccountsAreNotUsed) {
  auto const form = build_form(mapping(), {{"2510", 0}});
  EXPECT_EQ(used(form), "");
  EXPECT_TRUE(form.errors.empty());
}

TEST(Ink2Form, RoundingNotes) {
  // Each field rounds up by half a krona: tillgångar 2 kr against eget kapital 1 kr and debt 1 kr
  Balances const balances{{"1930", 150}, {"2081", -50}, {"2440", -50}, {"2099", -50}, {"3001", -50}, {"8999", 50}};
  auto const form = build_form(mapping(), balances);
  EXPECT_EQ(used(form), "7281=2 7301=1 7302=1 7365=1 7410=1 7450=1");
  EXPECT_EQ(form.notes,
            (std::vector<std::string>{"Rounding: tillgångar (72xx) add up to 2 kr but eget kapital och skulder (73xx) "
                                      "to 3 kr, as each field is rounded on its own"}));
}

TEST(Ink2Form, Sums) {
  Balances const balances{{"1930", 110000}, {"2440", -20000}, {"2081", -80000}, {"2099", -10000},
                          {"3001", -100000}, {"5410", 90000}, {"8999", 10000}};
  auto const sums = cratchit::ink2::sums_of(build_form(mapping(), balances));
  EXPECT_EQ(sums.assets, 1100);
  EXPECT_EQ(sums.equity_and_debt, 1100);
  EXPECT_EQ(sums.result, 100);
  EXPECT_EQ(sums.year_result, 100);
}
