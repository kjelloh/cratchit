#include "core/ar/disposition.hpp"
#include "core/ar/notes.hpp"
#include "core/ar/overview.hpp"
#include "core/ar/text.hpp"
#include "core/csv.hpp"
#include "core/sie/file.hpp"

#include <gtest/gtest.h>

#include <charconv>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using cratchit::Amount;
using cratchit::Balances;
using cratchit::ar::build_disposition;
using cratchit::ar::build_overview;
using cratchit::ar::format_percent;
using cratchit::ar::to_tkr;
using namespace std::chrono;

namespace {

std::vector<cratchit::ar::Line> layout() {
  auto const lines = cratchit::ar::parse_layout("Nivå;Typ;BAS Range;ÅR\n"
                                                "1;Rubrik;;Resultaträkning\n"
                                                "3;Post;3000-3999;Nettoomsättning\n"
                                                "3;Post;8300-8399;Ränteintäkter\n"
                                                "1;Summa;;Resultat efter finansiella poster\n"
                                                "1;Rubrik;;Balansräkning\n"
                                                "2;Rubrik;;Tillgångar\n"
                                                "3;Post;1220-1229;Inventarier, verktyg och installationer\n"
                                                "3;Post;1930;Kassa och bank\n"
                                                "2;Summa;;Summa tillgångar\n"
                                                "2;Rubrik;;Eget kapital och skulder\n"
                                                "3;Rubrik;;Eget kapital\n"
                                                "4;Rubrik;;Fritt eget kapital\n"
                                                "5;Post;2097;Fri överkursfond\n"
                                                "5;Post;2091;Balanserat resultat\n"
                                                "5;Post;2099;Årets resultat\n"
                                                "4;Summa;;Summa fritt eget kapital\n"
                                                "3;Summa;;Summa eget kapital\n"
                                                "3;Rubrik;;Obeskattade reserver\n"
                                                "4;Post;2110-2149;Periodiseringsfonder\n"
                                                "3;Summa;;Summa obeskattade reserver\n"
                                                "3;Post;2440;Leverantörsskulder\n"
                                                "2;Summa;;Summa eget kapital och skulder\n"
                                                "1;Rubrik;;Noter\n"
                                                "2;Rubrik;;Not 3 Inventarier, verktyg och installationer\n"
                                                "3;Post;1220-1227;Anskaffningsvärden\n"
                                                "2;Summa;;Redovisat värde\n");
  EXPECT_TRUE(lines);
  return *lines;
}

// Assets 1 000 kr; equity 600 kr (of which this year's result 150 kr); a periodiseringsfond 100 kr; debt 300 kr
Balances year0() {
  return {{"3001", -149950}, {"8310", -50}, {"1930", 100000}, {"2091", -45000}, {"2099", -15000},
          {"2110", -10000},  {"2440", -30000}};
}

// No assets at all
Balances year1() {
  return {{"3001", -40000}, {"2091", 40000}};
}

} // namespace

TEST(ArOverview, ToTkrRoundsHalfAwayFromZero) {
  EXPECT_EQ(to_tkr(152672), 2);
  EXPECT_EQ(to_tkr(33582500), 336);
  EXPECT_EQ(to_tkr(-8419250), -84);
  EXPECT_EQ(to_tkr(49999), 0);
  EXPECT_EQ(to_tkr(50000), 1);
  EXPECT_EQ(to_tkr(-50000), -1);
  EXPECT_EQ(to_tkr(-400), 0);
}

TEST(ArOverview, FormatPercent) {
  EXPECT_EQ(format_percent(962), "96,2");
  EXPECT_EQ(format_percent(1000), "100,0");
  EXPECT_EQ(format_percent(5), "0,5");
  EXPECT_EQ(format_percent(-123), "-12,3");
}

TEST(ArOverview, KeyFigures) {
  auto const statement = cratchit::ar::build_statement(layout(), {year0(), year1()});
  auto const overview = build_overview(statement);
  EXPECT_EQ(overview.net_sales, (std::vector<Amount>{149950, 40000}));
  EXPECT_EQ(overview.result_after_financial_items, (std::vector<Amount>{150000, 40000}));
  EXPECT_EQ(overview.equity, (std::vector<Amount>{60000, -40000}));
  EXPECT_EQ(overview.untaxed_reserves, (std::vector<Amount>{10000, 0}));
  EXPECT_EQ(overview.assets, (std::vector<Amount>{100000, 0}));
  // (600 + 0,794 * 100) / 1 000 = 67,94 %
  EXPECT_EQ(overview.solidity[0], 679);
  EXPECT_FALSE(overview.solidity[1]);
  auto const text = cratchit::ar::format_overview(
      overview, {{0, 2023y / May / 1, 2024y / April / 30}, {-1, 2022y / May / 1, 2023y / April / 30}}, {});
  EXPECT_NE(text.find("\nNettoomsättning (tkr)                                                1             0\n"),
            std::string::npos)
      << text;
  EXPECT_NE(text.find("\nSoliditet (%)                                                     67,9             -\n"),
            std::string::npos)
      << text;
}

TEST(ArDisposition, FundsAreFreeEquity) {
  auto const statement = cratchit::ar::build_statement(layout(), {year0(), year1()});
  auto const disposition = build_disposition(statement);
  ASSERT_EQ(disposition.funds.size(), 2u); // no Fri överkursfond
  EXPECT_EQ(disposition.funds[0].text, "Balanserat resultat");
  EXPECT_EQ(disposition.funds[0].amount, 45000);
  EXPECT_EQ(disposition.funds[1].text, "Årets resultat");
  EXPECT_EQ(disposition.funds[1].amount, 15000);
  EXPECT_EQ(disposition.total, 60000);
  auto const statement_check = cratchit::ar::check_requirements(statement, build_overview(statement), {});
  EXPECT_EQ(cratchit::ar::format_disposition(disposition, statement_check),
            "Resultatdisposition\n"
            "-------------------\n"
            "\n"
            "Till årsstämmans förfogande står följande medel\n"
            "  Balanserat resultat                                              450\n"
            "  Årets resultat                                                   150\n"
            "Summa                                                              600\n"
            "\n"
            "Styrelsen föreslår att medlen disponeras enligt följande\n"
            "  Utdelas till aktieägare                                      TBD [6]\n"
            "  Balanseras i ny räkning                                      TBD [6]\n"
            "Summa                                                              600\n");
}

TEST(ArNotes, ReferenceInTheBalanceSheet) {
  cratchit::sie::Document document;
  document.opening = {{0, "1221", 50000, ""}};
  document.closing = {{0, "1221", 50000, ""}};
  auto const statement = cratchit::ar::build_statement(layout(), {Balances{{"1221", 50000}, {"2091", -50000}}});
  auto const notes = cratchit::ar::build_notes(statement, document);
  auto const references = cratchit::ar::note_references(notes);
  EXPECT_EQ(references, (std::map<std::string, std::string>{{"Inventarier, verktyg och installationer", "3"}}));
  auto const text = cratchit::ar::format_statements(statement, {{0, 2023y / May / 1, 2024y / April / 30}}, references);
  EXPECT_NE(text.find("\n                                                     Not    2024-04-30\n"), std::string::npos) << text;
  EXPECT_NE(text.find("\n  Inventarier, verktyg och installationer              3           500\n"), std::string::npos)
      << text;
  // No Not column without notes
  EXPECT_EQ(cratchit::ar::format_statements(statement, {{0, 2023y / May / 1, 2024y / April / 30}}).find("Not "),
            std::string::npos);
}

namespace {

// The rows of an expected file, without its # comment lines and header
std::vector<cratchit::CsvRow> expected_rows(std::string const& name) {
  std::ifstream in(std::filesystem::path(CRATCHIT_AR_EXPECTED_FOLDER) / name);
  EXPECT_TRUE(in) << name;
  std::string text;
  for (std::string line; std::getline(in, line);) {
    if (!line.starts_with('#')) {
      text += line + '\n';
    }
  }
  auto rows = cratchit::parse_csv(text, ';');
  EXPECT_TRUE(rows);
  if (!rows || rows->empty()) {
    return {};
  }
  rows->erase(rows->begin());
  return *rows;
}

// A number of kronor, tkr or tenths of a percent as typed in: "-84", "96,2"
std::int64_t number_of(std::string text) {
  std::erase(text, ',');
  std::int64_t value = 0;
  std::from_chars(text.data(), text.data() + text.size(), value);
  return value;
}

class ArDirectorsReportAgainstPdf : public testing::TestWithParam<std::pair<char const*, char const*>> {};

} // namespace

TEST_P(ArDirectorsReportAgainstPdf, MatchesWithinARoundingStep) {
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
  std::string const period = GetParam().second;

  auto const overview = build_overview(statement);
  auto const figures = expected_rows(period + "_flerarsoversikt.csv");
  ASSERT_EQ(figures.size(), 3u);
  for (std::size_t year = 0; year < 2; ++year) {
    EXPECT_LE(std::abs(to_tkr(overview.net_sales[year]) - number_of(figures[0][1 + year])), 1) << year;
    EXPECT_LE(std::abs(to_tkr(overview.result_after_financial_items[year]) - number_of(figures[1][1 + year])), 1) << year;
    ASSERT_TRUE(overview.solidity[year]);
    EXPECT_LE(std::abs(*overview.solidity[year] - number_of(figures[2][1 + year])), 1) << year;
  }

  auto const disposition = build_disposition(statement);
  auto const funds = expected_rows(period + "_resultatdisposition.csv");
  ASSERT_EQ(funds.size(), disposition.funds.size() + 1);
  for (std::size_t index = 0; index < disposition.funds.size(); ++index) {
    EXPECT_EQ(funds[index][0], disposition.funds[index].text);
    EXPECT_LE(std::abs(cratchit::to_kronor(disposition.funds[index].amount) - number_of(funds[index][1])), 1);
  }
  EXPECT_EQ(funds.back()[0], "Summa");
  EXPECT_LE(std::abs(cratchit::to_kronor(disposition.total) - number_of(funds.back()[1])), 1);
}

INSTANTIATE_TEST_SUITE_P(Filed, ArDirectorsReportAgainstPdf,
                         testing::Values(std::pair{"TheITfiedAB20230503_152345.se", "220501-230430"},
                                         std::pair{"TheITfiedAB20240727_213539.se", "230501-240430"},
                                         std::pair{"TheITfiedAB20250817_172409.se", "240501-250430"}));
