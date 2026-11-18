#include "core/ar/layout.hpp"
#include "core/ar/statement.hpp"
#include "core/ar/text.hpp"
#include "core/csv.hpp"
#include "core/sie/document.hpp"
#include "core/sie/file.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

using cratchit::Balances;
using cratchit::ar::build_statement;
using cratchit::ar::check_statement;
using cratchit::ar::format_kronor;
using cratchit::Kronor;
using cratchit::ar::Statement;
using cratchit::to_kronor;

namespace {

// A small resultaträkning and balansräkning
std::vector<cratchit::ar::Line> small_layout() {
  auto const lines = cratchit::ar::parse_layout("Nivå;Typ;BAS Range;ÅR\n"
                                                "1;Rubrik;;Resultaträkning\n"
                                                "2;Rubrik;;Intäkter\n"
                                                "3;Post;3000-3999;Nettoomsättning\n"
                                                "2;Summa;;Summa intäkter\n"
                                                "2;Rubrik;;Kostnader\n"
                                                "3;Post;5000-6999;Övriga externa kostnader\n"
                                                "3;Post;7000-7699;Personalkostnader\n"
                                                "2;Summa;;Summa kostnader\n"
                                                "1;Summa;;Årets resultat\n"
                                                "1;Rubrik;;Balansräkning\n"
                                                "2;Rubrik;;Tillgångar\n"
                                                "3;Post;1900-1999;Kassa och bank\n"
                                                "2;Summa;;Summa tillgångar\n"
                                                "2;Rubrik;;Eget kapital och skulder\n"
                                                "3;Post;2081;Aktiekapital\n"
                                                "3;Post;2099;Årets resultat\n"
                                                "3;Post;2440;Leverantörsskulder\n"
                                                "2;Summa;;Summa eget kapital och skulder\n"
                                                ";Utanför;8999;Bokslutspost\n");
  EXPECT_TRUE(lines);
  return *lines;
}

Kronor shown(Statement const& statement, std::string_view text) {
  auto const found = std::ranges::find(statement.lines, text, &cratchit::ar::StatementLine::text);
  EXPECT_NE(found, statement.lines.end()) << text;
  return found == statement.lines.end() ? 0 : found->amounts.front();
}

bool is_shown(Statement const& statement, std::string_view text) {
  return std::ranges::find(statement.lines, text, &cratchit::ar::StatementLine::text)->shown;
}

} // namespace

TEST(ArStatement, ToKronorRoundsHalfAwayFromZero) {
  EXPECT_EQ(to_kronor(152672), 1527);
  EXPECT_EQ(to_kronor(19550), 196);
  EXPECT_EQ(to_kronor(19549), 195);
  EXPECT_EQ(to_kronor(-19550), -196);
  EXPECT_EQ(to_kronor(-4), 0);
}

TEST(ArStatement, FormatKronor) {
  EXPECT_EQ(format_kronor(0), "0");
  EXPECT_EQ(format_kronor(-84193), "-84 193");
  EXPECT_EQ(format_kronor(100800), "100 800");
  EXPECT_EQ(format_kronor(1234567), "1 234 567");
  EXPECT_EQ(format_kronor(-100), "-100");
}

TEST(ArStatement, SignsSumsAndChecks) {
  // Share capital 100 paid in, sold for 1 000, paid 300 in costs, 400 still owed for a cost;
  // the result 300 booked to 2099. Cash 800 = 100 + 300 + 400.
  Balances const year{{"3001", -100000}, {"5410", 30000},  {"6212", 40000},  {"8999", -30000},
                      {"1930", 80000},   {"2081", -10000}, {"2099", -30000}, {"2440", -40000}};
  auto const statement = build_statement(small_layout(), {year});
  EXPECT_TRUE(statement.errors.empty());
  EXPECT_EQ(shown(statement, "Nettoomsättning"), 1000);
  EXPECT_EQ(shown(statement, "Övriga externa kostnader"), -700);
  EXPECT_EQ(shown(statement, "Summa kostnader"), -700);
  EXPECT_EQ(shown(statement, "Årets resultat"), 300);
  EXPECT_EQ(shown(statement, "Summa tillgångar"), 800);
  EXPECT_EQ(shown(statement, "Leverantörsskulder"), 400);
  EXPECT_EQ(shown(statement, "Summa eget kapital och skulder"), 800);
  EXPECT_TRUE(check_statement(statement).empty());
  EXPECT_FALSE(is_shown(statement, "Personalkostnader"));
  EXPECT_TRUE(is_shown(statement, "Kostnader"));
}

TEST(ArStatement, AccountOnTwoLinesIsAnError) {
  auto const layout = cratchit::ar::parse_layout("Nivå;Typ;BAS Range;ÅR\n1;Rubrik;;Balansräkning\n"
                                                 "3;Post;2650;A\n3;Post;2600-2699;B\n");
  ASSERT_TRUE(layout);
  auto const statement = build_statement(*layout, {Balances{{"2650", 100}}, Balances{{"2650", 200}}});
  ASSERT_EQ(statement.errors.size(), 1u);
  EXPECT_EQ(statement.errors.front(), "Account 2650 is on more than one line: A and B");
}

TEST(ArStatement, AccountMayAlsoBeOnALineOfTheNotes) {
  auto const layout = cratchit::ar::parse_layout("Nivå;Typ;BAS Range;ÅR\n1;Rubrik;;Balansräkning\n"
                                                 "3;Post;1220-1229;Inventarier\n"
                                                 "1;Rubrik;;Noter\n2;Rubrik;;Not 3 Inventarier\n"
                                                 "3;Post;1220-1227;Anskaffningsvärden\n3;Post;1229;Avskrivningar\n"
                                                 "3;Post;1220-1229;Fel\n2;Summa;;Redovisat värde\n");
  ASSERT_TRUE(layout);
  auto const statement = build_statement(*layout, {Balances{{"1221", 10000}, {"1229", -2000}}});
  ASSERT_EQ(statement.errors.size(), 2u);
  EXPECT_EQ(statement.errors[0], "Account 1221 is on more than one line of the notes: Anskaffningsvärden and Fel");
  EXPECT_EQ(statement.errors[1], "Account 1229 is on more than one line of the notes: Avskrivningar and Fel");
  auto const inventarier = std::ranges::find(statement.lines, "Inventarier", &cratchit::ar::StatementLine::text);
  EXPECT_FALSE(inventarier->in_notes);
  EXPECT_EQ(inventarier->amounts.front(), 80);
  auto const sum = std::ranges::find(statement.lines, "Redovisat värde", &cratchit::ar::StatementLine::text);
  EXPECT_TRUE(sum->in_notes);
}

TEST(ArStatement, HeadingWithoutItemsIsNotShown) {
  Balances const year{{"1930", 100}, {"2081", -100}};
  auto const statement = build_statement(small_layout(), {year});
  EXPECT_FALSE(is_shown(statement, "Intäkter"));
  EXPECT_FALSE(is_shown(statement, "Summa intäkter"));
  EXPECT_TRUE(is_shown(statement, "Tillgångar"));
}

TEST(ArStatement, AccountOnNoLineIsAnError) {
  Balances const year{{"1930", 100}, {"2081", -50}, {"2999", -50}};
  auto const statement = build_statement(small_layout(), {year});
  ASSERT_EQ(statement.errors.size(), 1u);
  EXPECT_NE(statement.errors.front().find("2999"), std::string::npos);
}

TEST(ArStatement, ZeroAmountNeedsNoLine) {
  Balances const year{{"1930", 100}, {"2081", -100}, {"2999", 0}};
  EXPECT_TRUE(build_statement(small_layout(), {year}).errors.empty());
}

TEST(ArStatement, NotBalancingIsFound) {
  Balances const year{{"1930", 100}, {"2081", -50}};
  auto const findings = check_statement(build_statement(small_layout(), {year}));
  ASSERT_EQ(findings.size(), 1u);
  EXPECT_NE(findings.front().find("Summa tillgångar"), std::string::npos);
}

TEST(ArStatement, UnbookedResultIsFound) {
  // The result 300 is not booked to 2099, so the two Årets resultat differ
  Balances const year{{"3001", -30000}, {"1930", 40000}, {"2081", -10000}};
  auto const findings = check_statement(build_statement(small_layout(), {year}));
  EXPECT_EQ(findings.size(), 2u);
}

namespace {

// The SIE files and the årsredovisningar they were filed with, in the session's
// _Resources (git ignored, so the tests skip when they are missing)
struct Filed {
  char const* sie;
  char const* expected;
};

class ArAgainstPdf : public testing::TestWithParam<Filed> {};

} // namespace

TEST_P(ArAgainstPdf, AmountsMatchWithinAKrona) {
  auto const sie = std::filesystem::path(CRATCHIT_AR_SIE_FOLDER) / GetParam().sie;
  if (!std::filesystem::exists(sie)) {
    GTEST_SKIP() << "No " << sie;
  }
  auto const lexed = cratchit::sie::lex_file(sie);
  ASSERT_TRUE(lexed);
  auto const parsed = cratchit::sie::parse_document(lexed->lines);
  ASSERT_TRUE(parsed.errors.empty());
  auto const layout = cratchit::ar::parse_layout(cratchit::ar::layout_text());
  ASSERT_TRUE(layout);
  auto const statement = build_statement(*layout, {cratchit::year_balances(parsed.document, 0),
                                                   cratchit::year_balances(parsed.document, -1)});
  EXPECT_TRUE(statement.errors.empty());
  EXPECT_TRUE(check_statement(statement).empty());

  std::ifstream in(std::filesystem::path(CRATCHIT_AR_EXPECTED_FOLDER) / GetParam().expected);
  ASSERT_TRUE(in);
  std::string text;
  for (std::string line; std::getline(in, line);) {
    if (!line.starts_with('#')) {
      text += line + '\n';
    }
  }
  auto const rows = cratchit::parse_csv(text, ';');
  ASSERT_TRUE(rows);
  ASSERT_GT(rows->size(), 1u);
  // Lines are found in order, so that a text that is on two lines is the one after the line before
  auto from = statement.lines.begin();
  for (std::size_t index = 1; index < rows->size(); ++index) {
    auto const& row = (*rows)[index];
    ASSERT_GE(row.size(), 3u);
    auto const found = std::find_if(from, statement.lines.end(), [&row](auto const& line) {
      return line.text == row[0] && line.kind != cratchit::ar::LineKind::heading;
    });
    ASSERT_NE(found, statement.lines.end()) << row[0] << " not found after the line before";
    for (std::size_t year = 0; year < 2; ++year) {
      Kronor expected = 0;
      std::from_chars(row[1 + year].data(), row[1 + year].data() + row[1 + year].size(), expected);
      EXPECT_LE(std::abs(found->amounts[year] - expected), 1)
          << row[0] << " year " << -static_cast<int>(year) << ": " << found->amounts[year] << " against " << expected;
    }
    from = found + 1;
  }
}

INSTANTIATE_TEST_SUITE_P(Filed, ArAgainstPdf,
                         testing::Values(Filed{"TheITfiedAB20230503_152345.se", "220501-230430.csv"},
                                         Filed{"TheITfiedAB20240727_213539.se", "230501-240430.csv"},
                                         Filed{"TheITfiedAB20250817_172409.se", "240501-250430.csv"}));
