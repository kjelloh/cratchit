#include "core/ar/equity.hpp"
#include "core/ar/text.hpp"
#include "core/csv.hpp"
#include "core/sie/file.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using cratchit::Amount;
using cratchit::ar::equity_changes;
using cratchit::ar::EquityChanges;
using cratchit::sie::Transaction;
using namespace std::chrono;

namespace {

std::vector<cratchit::ar::Line> equity_layout() {
  auto const lines = cratchit::ar::parse_layout("Nivå;Typ;BAS Range;ÅR\n"
                                                "1;Rubrik;;Balansräkning\n"
                                                "3;Rubrik;;Eget kapital\n"
                                                "4;Rubrik;;Bundet eget kapital\n"
                                                "5;Post;2081;Aktiekapital\n"
                                                "5;Post;2085;Uppskrivningsfond\n"
                                                "4;Summa;;Summa bundet eget kapital\n"
                                                "4;Rubrik;;Fritt eget kapital\n"
                                                "5;Post;2091, 2093 eller 2098;Balanserat resultat\n"
                                                "5;Post;2099;Årets resultat\n"
                                                "4;Summa;;Summa fritt eget kapital\n"
                                                "3;Summa;;Summa eget kapital\n"
                                                "3;Rubrik;;Kortfristiga skulder\n"
                                                "4;Post;2898;Övriga skulder\n"
                                                "3;Summa;;Summa kortfristiga skulder\n");
  EXPECT_TRUE(lines);
  return *lines;
}

cratchit::sie::Voucher voucher(std::string number, std::vector<std::pair<std::string, Amount>> const& postings,
                               Transaction::Kind kind = Transaction::Kind::normal) {
  cratchit::sie::Voucher out{"I", std::move(number), 2023y / May / 1, "text", std::nullopt, "", {}, 0};
  for (auto const& [account, amount] : postings) {
    Transaction transaction;
    transaction.kind = kind;
    transaction.account = account;
    transaction.amount = amount;
    out.transactions.push_back(transaction);
  }
  return out;
}

// Opening 2081 -100, 2091 -50, 2099 -30 (last year's profit 30); this year a loss of 20
cratchit::sie::Document books() {
  cratchit::sie::Document document;
  document.opening = {{0, "2081", -10000, ""}, {0, "2091", -5000, ""}, {0, "2099", -3000, ""}};
  document.vouchers = {
      voucher("1", {{"2099", 3000}, {"2098", -3000}}),                     // last year's profit to 2098
      voucher("2", {{"2098", 3000}, {"2898", -1000}, {"2091", -2000}}),    // dividend 10, rest to 2091
      voucher("3", {{"2099", 2000}, {"8999", -2000}}),                     // this year's loss
  };
  document.closing = {{0, "2081", -10000, ""}, {0, "2091", -7000, ""}, {0, "2098", 0, ""}, {0, "2099", 2000, ""}};
  return document;
}

std::vector<Amount> row_of(EquityChanges const& changes, std::string_view text) {
  auto const found = std::ranges::find(changes.rows, text, &EquityChanges::Row::text);
  EXPECT_NE(found, changes.rows.end()) << text;
  return found == changes.rows.end() ? std::vector<Amount>{} : found->amounts;
}

bool has_row(EquityChanges const& changes, std::string_view text) {
  return std::ranges::find(changes.rows, text, &EquityChanges::Row::text) != changes.rows.end();
}

} // namespace

TEST(ArEquity, ColumnsAreThePostsWithAnAmount) {
  auto const changes = equity_changes(equity_layout(), books());
  EXPECT_EQ(changes.columns, (std::vector<std::string>{"Aktiekapital", "Balanserat resultat", "Årets resultat"}));
}

TEST(ArEquity, RowsByCounterAccount) {
  auto const changes = equity_changes(equity_layout(), books());
  EXPECT_TRUE(changes.errors.empty());
  EXPECT_TRUE(changes.notes.empty());
  ASSERT_EQ(changes.rows.size(), 5u);
  EXPECT_EQ(changes.rows.front().text, "Belopp vid årets ingång");
  EXPECT_EQ(changes.rows.back().text, "Belopp vid årets utgång");
  EXPECT_EQ(row_of(changes, "Belopp vid årets ingång"), (std::vector<Amount>{10000, 5000, 3000, 18000}));
  EXPECT_EQ(row_of(changes, "Utdelning"), (std::vector<Amount>{0, -1000, 0, -1000}));
  EXPECT_EQ(row_of(changes, "Balanseras i ny räkning"), (std::vector<Amount>{0, 3000, -3000, 0}));
  EXPECT_EQ(row_of(changes, "Årets resultat"), (std::vector<Amount>{0, 0, -2000, -2000}));
  EXPECT_EQ(row_of(changes, "Belopp vid årets utgång"), (std::vector<Amount>{10000, 7000, -2000, 15000}));
}

TEST(ArEquity, CancelledDividendIsNotShown) {
  auto document = books();
  document.vouchers.push_back(voucher("4", {{"2091", 1000}, {"2898", -1000}}));
  document.vouchers.push_back(voucher("5", {{"2091", -1000}, {"2898", 1000}}));
  auto const changes = equity_changes(equity_layout(), document);
  EXPECT_EQ(row_of(changes, "Utdelning"), (std::vector<Amount>{0, -1000, 0, -1000}));
  document.vouchers.erase(document.vouchers.begin() + 1); // only the cancelled one is left
  document.closing[1].amount = -5000;
  document.closing[2].amount = -3000;
  auto const cancelled = equity_changes(equity_layout(), document);
  EXPECT_FALSE(has_row(cancelled, "Utdelning"));
  EXPECT_TRUE(cancelled.errors.empty()) << cancelled.errors.front();
}

TEST(ArEquity, ShareholderContribution) {
  auto document = books();
  document.vouchers.push_back(voucher("4", {{"1930", 5000000}, {"2093", -5000000}}));
  document.closing[1].amount -= 5000000;
  auto const changes = equity_changes(equity_layout(), document);
  EXPECT_TRUE(changes.errors.empty());
  EXPECT_EQ(row_of(changes, "Erhållna aktieägartillskott"), (std::vector<Amount>{0, 5000000, 0, 5000000}));
  EXPECT_EQ(changes.rows[1].text, "Erhållna aktieägartillskott");
}

TEST(ArEquity, OtherChangeIsNoted) {
  auto document = books();
  auto other = voucher("4", {{"1930", 500}, {"2091", -500}});
  other.text = "Okänd insättning";
  document.vouchers.push_back(other);
  document.closing[1].amount -= 500;
  auto const changes = equity_changes(equity_layout(), document);
  EXPECT_EQ(row_of(changes, "Övriga förändringar"), (std::vector<Amount>{0, 500, 0, 500}));
  ASSERT_EQ(changes.notes.size(), 1u);
  EXPECT_EQ(changes.notes.front(),
            "Voucher I 4 2023-05-01 \"Okänd insättning\" changes eget kapital; shown as Övriga förändringar");
}

TEST(ArEquity, RemovedTransactionsDoNotCount) {
  auto document = books();
  document.vouchers.push_back(voucher("4", {{"2091", 1000}, {"2898", -1000}}, Transaction::Kind::removed));
  EXPECT_EQ(row_of(equity_changes(equity_layout(), document), "Utdelning"), (std::vector<Amount>{0, -1000, 0, -1000}));
}

TEST(ArEquity, ClosingThatDoesNotAddUpIsAnError) {
  auto document = books();
  document.closing[1].amount = -6900;
  auto const changes = equity_changes(equity_layout(), document);
  ASSERT_EQ(changes.errors.size(), 1u);
  EXPECT_EQ(changes.errors.front(),
            "Förändringar i eget kapital: Balanserat resultat opens at 50.00 and changes to 70.00, but closes at 69.00");
}

TEST(ArEquity, ZeroCellsOfAChangeAreBlank) {
  auto const text = cratchit::ar::format_equity_changes(equity_changes(equity_layout(), books()));
  EXPECT_NE(text.find("\nÅrets resultat                                                       -20           -20\n"),
            std::string::npos)
      << text;
  EXPECT_NE(text.find("\nBelopp vid årets ingång                  100            50            30           180\n"),
            std::string::npos)
      << text;
}

namespace {

class ArEquityAgainstPdf : public testing::TestWithParam<std::pair<char const*, char const*>> {};

} // namespace

TEST_P(ArEquityAgainstPdf, AmountsMatchWithinAKrona) {
  auto const sie = std::filesystem::path(CRATCHIT_AR_SIE_FOLDER) / GetParam().first;
  if (!std::filesystem::exists(sie)) {
    GTEST_SKIP() << "No " << sie;
  }
  auto const lexed = cratchit::sie::lex_file(sie);
  ASSERT_TRUE(lexed);
  auto const layout = cratchit::ar::parse_layout(cratchit::ar::layout_text());
  ASSERT_TRUE(layout);
  auto const changes = equity_changes(*layout, cratchit::sie::parse_document(lexed->lines).document);
  EXPECT_TRUE(changes.errors.empty());
  EXPECT_TRUE(changes.notes.empty());

  std::ifstream in(std::filesystem::path(CRATCHIT_AR_EXPECTED_FOLDER) / GetParam().second);
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
  // The header names the columns, then Totalt and Kommentar
  auto const& header = rows->front();
  ASSERT_EQ(header.size(), changes.columns.size() + 3);
  for (std::size_t column = 0; column < changes.columns.size(); ++column) {
    EXPECT_EQ(header[column + 1], changes.columns[column]);
  }
  // Exactly the expected rows, in order
  ASSERT_EQ(rows->size() - 1, changes.rows.size());
  for (std::size_t index = 1; index < rows->size(); ++index) {
    auto const& row = (*rows)[index];
    auto const& shown = changes.rows[index - 1];
    EXPECT_EQ(row[0], shown.text);
    for (std::size_t column = 0; column < shown.amounts.size(); ++column) {
      cratchit::Kronor expected = 0;
      std::from_chars(row[column + 1].data(), row[column + 1].data() + row[column + 1].size(), expected);
      EXPECT_LE(std::abs(cratchit::to_kronor(shown.amounts[column]) - expected), 1)
          << row[0] << " column " << column << ": " << cratchit::to_kronor(shown.amounts[column]) << " against "
          << expected;
    }
  }
}

INSTANTIATE_TEST_SUITE_P(Filed, ArEquityAgainstPdf,
                         testing::Values(std::pair{"TheITfiedAB20230503_152345.se", "220501-230430_eget_kapital.csv"},
                                         std::pair{"TheITfiedAB20240727_213539.se", "230501-240430_eget_kapital.csv"},
                                         std::pair{"TheITfiedAB20250817_172409.se", "240501-250430_eget_kapital.csv"}));
