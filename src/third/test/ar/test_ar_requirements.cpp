#include "core/ar/requirements.hpp"
#include "core/ar/text.hpp"
#include "core/sie/file.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <set>
#include <string>
#include <vector>

using cratchit::Balances;
using cratchit::ar::Check;
using cratchit::ar::check_requirements;
using namespace std::chrono;

namespace {

std::vector<cratchit::ar::Line> layout() {
  auto const lines = cratchit::ar::parse_layout("Nivå;Typ;BAS Range;ÅR\n"
                                                "1;Rubrik;;Resultaträkning\n"
                                                "3;Post;3000-3999;Nettoomsättning\n"
                                                "3;Post;7000-7699;Personalkostnader\n"
                                                "3;Post;7800-7899;Av- och nedskrivningar av materiella och immateriella "
                                                "anläggningstillgångar\n"
                                                "1;Rubrik;;Balansräkning\n"
                                                "4;Rubrik;;Materiella anläggningstillgångar\n"
                                                "5;Post;1210-1219;Maskiner och andra tekniska anläggningar\n"
                                                "4;Summa;;Summa materiella anläggningstillgångar\n"
                                                "3;Post;1930;Kassa och bank\n"
                                                "3;Post;2081;Aktiekapital\n");
  EXPECT_TRUE(lines);
  return *lines;
}

Check check_of(Balances const& year0, Balances const& year1) {
  auto const statement = cratchit::ar::build_statement(layout(), {year0, year1});
  return check_requirements(statement, cratchit::ar::build_overview(statement), {});
}

std::vector<std::string> ids_of(std::vector<cratchit::ar::CheckItem> const& items) {
  std::vector<std::string> ids;
  for (auto const& item : items) {
    ids.emplace_back(item.requirement->id);
  }
  return ids;
}

// Sales of 1 000 kr both years, nothing else of note
Balances quiet() {
  return {{"3001", -100000}, {"1930", 100000}, {"2081", -100000}};
}

} // namespace

TEST(ArRequirements, IdsAreUniqueAndCited) {
  std::set<std::string_view> ids;
  for (auto const& requirement : cratchit::ar::requirements()) {
    EXPECT_TRUE(ids.insert(requirement.id).second) << requirement.id;
    EXPECT_FALSE(requirement.title.empty());
    EXPECT_FALSE(requirement.rule.empty()) << requirement.id;
    EXPECT_FALSE(requirement.provide.empty()) << requirement.id;
  }
}

TEST(ArRequirements, AQuietCompany) {
  auto const check = check_of(quiet(), quiet());
  EXPECT_EQ(ids_of(check.missing), (std::vector<std::string>{"sate", "verksamhet", "handelser", "flerar", "forslag",
                                                              "datering", "underskrifter"}));
  EXPECT_EQ(ids_of(check.to_confirm), (std::vector<std::string>{"omsattning", "principer", "anstallda", "yttrande",
                                                                 "sakerheter", "eventual", "styrelselan", "efter"}));
  for (std::size_t index = 0; index < check.missing.size(); ++index) {
    EXPECT_EQ(check.missing[index].number, static_cast<int>(index) + 1);
  }
  EXPECT_EQ(check.to_confirm.front().number, 8);
  EXPECT_EQ(check.marker("forslag"), "TBD [5]");
  EXPECT_FALSE(check.marker("yttrande"));
  EXPECT_FALSE(check.marker("okänd"));
}

TEST(ArRequirements, WhatTheBooksShowMakesItRequired) {
  auto year0 = quiet();
  year0["3001"] = -140000;  // 40 % more sales
  year0["7010"] = 20000;    // salaries
  year0["7832"] = 10000;    // depreciation
  year0["1211"] = 50000;    // a machine without a note
  year0["1930"] = 20000;
  auto const check = check_of(year0, quiet());
  auto const missing = ids_of(check.missing);
  EXPECT_EQ(missing, (std::vector<std::string>{"sate", "verksamhet", "handelser", "flerar", "omsattning", "forslag",
                                               "principer", "anstallda", "not_post", "datering", "underskrifter"}));
  auto const& sales = *std::ranges::find(check.missing, std::string_view("omsattning"),
                                         [](auto const& item) { return item.requirement->id; });
  EXPECT_EQ(sales.detail, "Nettoomsättning went from 1000 kr to 1400 kr");
  auto const& note = *std::ranges::find(check.missing, std::string_view("not_post"),
                                        [](auto const& item) { return item.requirement->id; });
  EXPECT_EQ(note.detail, "no note for Maskiner och andra tekniska anläggningar");
}

TEST(ArRequirements, ThirtyPercentIsTheLimit) {
  auto year0 = quiet();
  year0["3001"] = -130000; // exactly 30 % more
  EXPECT_FALSE(check_of(year0, quiet()).marker("omsattning"));
  year0["3001"] = -130001;
  EXPECT_TRUE(check_of(year0, quiet()).marker("omsattning"));
  year0["3001"] = -70000; // 30 % less
  EXPECT_FALSE(check_of(year0, quiet()).marker("omsattning"));
  // No sales either year: nothing to comment
  Balances const none{{"1930", 100000}, {"2081", -100000}};
  EXPECT_FALSE(check_of(none, none).marker("omsattning"));
}

TEST(ArRequirements, TextsCarryTheMarkers) {
  auto const check = check_of(quiet(), quiet());
  cratchit::sie::Document document;
  document.company_name = "Bolaget AB";
  document.orgnr = "556000-0000";
  std::vector<cratchit::sie::FiscalYear> const years{{0, 2024y / May / 1, 2025y / April / 30}};
  EXPECT_EQ(cratchit::ar::format_front(document, years, check),
            "Bolaget AB\n556000-0000\nÅrsredovisning för räkenskapsåret 2024-05-01 - 2025-04-30\n\n"
            "Styrelsen avger följande årsredovisning. Redovisningsvalutan är svenska kronor (SEK)\n"
            "och alla belopp anges i hela kronor om inte annat anges.\n");
  EXPECT_EQ(cratchit::ar::format_business(check),
            "Förvaltningsberättelse\n======================\n\nVerksamheten\n------------\n\n"
            "Allmänt om verksamheten\n  Säte: TBD [1]\n  TBD [2]\n\n"
            "Väsentliga händelser under räkenskapsåret\n  TBD [3]\n");
  auto const signatures = cratchit::ar::format_signatures(document, years, check);
  EXPECT_NE(signatures.find("Årsredovisning för Bolaget AB, 556000-0000\nAvseende räkenskapsåret 2024-05-01 - 2025-04-30\n"),
            std::string::npos);
  EXPECT_NE(signatures.find("Ort och datum: TBD [6]\n"), std::string::npos) << signatures;
  auto const summary = cratchit::ar::format_check(check);
  EXPECT_NE(summary.find("Not complete: 7 items are missing"), std::string::npos) << summary;
  EXPECT_NE(summary.find("\n[1] Säte - BFNAR 2016:10 p. 5.2; ÅRL 2 kap. 5 §\n"
                         "    Provide: the municipality where the company has its registered office (säte).\n"
                         "    Template heading: ## Säte\n"),
            std::string::npos)
      << summary;
  EXPECT_NE(summary.find("\n[8] Kommentar till nettoomsättningens förändring - BFNAR 2016:10 p. 5.5\n"
                         "    When: if it varies more than 30 % between any of the four years.\n"),
            std::string::npos)
      << summary;
}

TEST(ArRequirements, NothingMissingIsComplete) {
  EXPECT_NE(cratchit::ar::format_check({}).find("Complete: nothing is missing."), std::string::npos);
}

namespace {

class ArRequirementsOfFile : public testing::TestWithParam<std::pair<char const*, std::vector<std::string>>> {};

} // namespace

TEST_P(ArRequirementsOfFile, MissingItems) {
  auto const sie = std::filesystem::path(CRATCHIT_AR_SIE_FOLDER) / GetParam().first;
  if (!std::filesystem::exists(sie)) {
    GTEST_SKIP() << "No " << sie;
  }
  auto const lexed = cratchit::sie::lex_file(sie);
  ASSERT_TRUE(lexed);
  auto const document = cratchit::sie::parse_document(lexed->lines).document;
  auto const lines = cratchit::ar::parse_layout(cratchit::ar::layout_text());
  ASSERT_TRUE(lines);
  auto const statement = cratchit::ar::build_statement(
      *lines, {cratchit::year_balances(document, 0), cratchit::year_balances(document, -1)});
  auto const check = check_requirements(statement, cratchit::ar::build_overview(statement),
                                        cratchit::ar::build_notes(statement, document));
  EXPECT_EQ(ids_of(check.missing), GetParam().second);
}

// All three have a note (Not 3) and no personnel costs; Nettoomsättning varies in each (336, 2, 0, 0 tkr)
INSTANTIATE_TEST_SUITE_P(
    Filed, ArRequirementsOfFile,
    testing::Values(
        std::pair{"TheITfiedAB20230503_152345.se",
                  std::vector<std::string>{"sate", "verksamhet", "handelser", "flerar", "omsattning", "forslag",
                                           "principer", "datering", "underskrifter"}},
        std::pair{"TheITfiedAB20240727_213539.se",
                  std::vector<std::string>{"sate", "verksamhet", "handelser", "flerar", "omsattning", "forslag",
                                           "principer", "datering", "underskrifter"}},
        std::pair{"TheITfiedAB20250817_172409.se",
                  std::vector<std::string>{"sate", "verksamhet", "handelser", "flerar", "omsattning", "forslag",
                                           "principer", "datering", "underskrifter"}}));
