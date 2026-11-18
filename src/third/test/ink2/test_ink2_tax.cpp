#include "core/ink2/tax.hpp"

#include <gtest/gtest.h>

#include <algorithm>

using cratchit::Balances;
using cratchit::Kronor;
using cratchit::ink2::build_tax_return;
using cratchit::ink2::Previous;

namespace {

std::vector<cratchit::ink2::Line> const& mapping() {
  static auto const lines = *cratchit::ink2::parse_mapping(cratchit::ink2::mapping_text());
  return lines;
}

std::vector<cratchit::ink2::SkvField> const& skv() {
  static auto const fields = *cratchit::ink2::parse_skv_fields(cratchit::ink2::skv_fields_text());
  return fields;
}

cratchit::ink2::TaxReturn tax_of(Balances const& balances, Previous const& previous = {}) {
  return build_tax_return(skv(), cratchit::ink2::build_form(mapping(), balances), previous);
}

} // namespace

TEST(Ink2Tax, ProfitIsOverskott) {
  // Sales 1000, costs 900 of which 100 not deductible, skattefri ränta 10
  Balances const balances{{"3001", -100000}, {"5410", 80000}, {"6992", 10000}, {"8314", -1000}, {"8999", 11000}};
  auto const tax = tax_of(balances);
  EXPECT_TRUE(tax.errors.empty());
  EXPECT_EQ(tax.ink2s, (std::map<std::string, Kronor>{{"7650", 110}, {"7653", 100}, {"7754", 10}, {"7670", 200}}));
  EXPECT_EQ(tax.ink2, (std::map<std::string, Kronor>{{"7104", 200}}));
}

TEST(Ink2Tax, LossAndLastYearsDeficit) {
  // The filed 2023-24, chained from the filed 2022-23: 30547 + 15 - 625 + 176026
  Balances const balances{{"5410", 2993701}, {"6992", 62500}, {"8314", -1500}, {"8999", -3054701}};
  Previous const previous{176026, "last.sru", {"8041", "8045"}, std::nullopt};
  auto const tax = tax_of(balances, previous);
  EXPECT_EQ(tax.ink2s,
            (std::map<std::string, Kronor>{{"7750", 30547}, {"7653", 625}, {"7754", 15}, {"7763", 176026}, {"7770", 205963}}));
  EXPECT_EQ(tax.ink2, (std::map<std::string, Kronor>{{"7114", 205963}}));
  EXPECT_EQ(tax.ink2s_marks, (std::set<std::string>{"8041", "8045"}));
}

TEST(Ink2Tax, ProfitUsesTheDeficit) {
  Balances const balances{{"3001", -100000}, {"8999", 100000}};
  EXPECT_EQ(tax_of(balances, {300, "x", {}, std::nullopt}).ink2, (std::map<std::string, Kronor>{{"7104", 700}}));
  EXPECT_EQ(tax_of(balances, {1300, "x", {}, std::nullopt}).ink2, (std::map<std::string, Kronor>{{"7114", 300}}));
  EXPECT_EQ(tax_of(balances, {1000, "x", {}, std::nullopt}).ink2, (std::map<std::string, Kronor>{{"7104", 0}}));
}

TEST(Ink2Tax, TaxOnTheResultIsAddedBack) {
  Balances const balances{{"3001", -100000}, {"8910", 20600}, {"8999", 79400}};
  auto const tax = tax_of(balances);
  EXPECT_EQ(tax.ink2s.at("7651"), 206);
  EXPECT_EQ(tax.ink2s.at("7670"), 1000);
}

TEST(Ink2Tax, Notes) {
  auto const without = tax_of({{"3001", -100}, {"8999", 100}});
  EXPECT_EQ(without.notes, (std::vector<std::string>{"INK2S: no marks for uppdragstagare (8040/8041) or revision "
                                                     "(8044/8045): not in the books, and not from last year's return"}));
  auto const with = tax_of({{"3001", -100}, {"8999", 100}}, {0, "last.sru", {"8041", "8045"}, std::nullopt});
  EXPECT_EQ(with.notes, (std::vector<std::string>{"INK2S: the marks 8041, 8045 (uppdragstagare, revision) as last "
                                                  "year (last.sru)"}));
  auto const fund = tax_of({{"2125", -500000}, {"1930", 500000}});
  EXPECT_NE(std::ranges::find(fund.notes, "INK2S: the books have periodiseringsfonder; 4.6a (schablonintäkt) is not "
                                          "computed"),
            fund.notes.end());
}
