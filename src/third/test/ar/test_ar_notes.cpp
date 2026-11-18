#include "core/ar/notes.hpp"
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
using cratchit::Balances;
using cratchit::ar::build_notes;
using cratchit::ar::Notes;
using cratchit::sie::Transaction;
using namespace std::chrono;

namespace {

std::vector<cratchit::ar::Line> layout() {
  auto const lines = cratchit::ar::parse_layout("Nivå;Typ;BAS Range;ÅR\n"
                                                "1;Rubrik;;Balansräkning\n"
                                                "3;Post;1220-1229;Inventarier\n"
                                                "3;Post;1930;Kassa och bank\n"
                                                "1;Rubrik;;Noter\n"
                                                "2;Rubrik;;Not 3 Inventarier\n"
                                                "3;Post;1220-1227;Anskaffningsvärden\n"
                                                "3;Post;1229;Avskrivningar\n"
                                                "3;Post;1228;Nedskrivningar\n"
                                                "2;Summa;;Redovisat värde\n");
  EXPECT_TRUE(lines);
  return *lines;
}

cratchit::sie::Voucher voucher(std::string number, std::vector<std::pair<std::string, Amount>> const& postings) {
  cratchit::sie::Voucher out{"A", std::move(number), 2023y / June / 1, "text", std::nullopt, "", {}, 0};
  for (auto const& [account, amount] : postings) {
    Transaction transaction;
    transaction.account = account;
    transaction.amount = amount;
    out.transactions.push_back(transaction);
  }
  return out;
}

// Opening: a computer bought for 1 000, depreciated 400. In the year: a purchase of 500 of which 100 is
// returned, depreciation 300, and the old computer scrapped (cost 1 000, depreciation 400, loss 600).
struct Books {
  cratchit::sie::Document document;
  Notes notes;
};

Books books(Amount closing_cost = 40000) {
  Books out;
  auto& document = out.document;
  document.opening = {{0, "1221", 100000, ""}, {0, "1229", -40000, ""}};
  document.vouchers = {
      voucher("1", {{"1221", 50000}, {"1930", -50000}}),                    // purchase
      voucher("2", {{"1930", 10000}, {"1221", -10000}}),                    // return
      voucher("3", {{"7830", 30000}, {"1229", -30000}}),                    // depreciation
      voucher("4", {{"1229", 40000}, {"7973", 60000}, {"1221", -100000}}), // scrapping
  };
  document.closing = {{0, "1221", closing_cost, ""}, {0, "1229", -30000, ""}};
  auto const statement = cratchit::ar::build_statement(layout(), {Balances{{"1221", 40000}, {"1229", -30000}}});
  out.notes = build_notes(statement, document);
  return out;
}

std::vector<std::pair<std::string, Amount>> rows_of(cratchit::ar::NoteSection const& section) {
  std::vector<std::pair<std::string, Amount>> rows;
  for (auto const& row : section.rows) {
    rows.emplace_back(row.text, row.amount);
  }
  return rows;
}

} // namespace

TEST(ArNotes, SectionsFromOpeningVouchersAndClosing) {
  auto const [document, notes] = books();
  EXPECT_TRUE(notes.errors.empty()) << notes.errors.front();
  ASSERT_EQ(notes.notes.size(), 1u);
  auto const& note = notes.notes.front();
  EXPECT_EQ(note.title, "Not 3 Inventarier");
  ASSERT_EQ(note.sections.size(), 3u);
  using Rows = std::vector<std::pair<std::string, Amount>>;
  EXPECT_EQ(rows_of(note.sections[0]), (Rows{{"Ingående anskaffningsvärden", 100000},
                                             {"Inköp", 40000},
                                             {"Försäljningar och utrangeringar", -100000},
                                             {"Utgående anskaffningsvärden", 40000}}));
  EXPECT_EQ(rows_of(note.sections[1]), (Rows{{"Ingående avskrivningar", -40000},
                                             {"Återförda avskrivningar", 40000},
                                             {"Årets avskrivningar", -30000},
                                             {"Utgående avskrivningar", -30000}}));
  EXPECT_EQ(rows_of(note.sections[2]), (Rows{{"Ingående nedskrivningar", 0}, {"Utgående nedskrivningar", 0}}));
  EXPECT_EQ(note.sum_text, "Redovisat värde");
  EXPECT_EQ(note.sum, 10000);
  ASSERT_EQ(note.sections[0].rows[1].vouchers.size(), 2u); // the purchase and the return
  EXPECT_EQ(note.sections[0].rows[2].vouchers.front()->number, "4");
}

TEST(ArNotes, SectionThatDoesNotAddUpIsAnError) {
  auto const notes = books(45000).notes;
  ASSERT_FALSE(notes.errors.empty());
  EXPECT_EQ(notes.errors.front(), "Not 3 Inventarier: Anskaffningsvärden opens and changes to 400.00, but closes at 450.00");
}

TEST(ArNotes, SumThatDiffersFromItsPostIsAnError) {
  auto [document, notes] = books();
  // The balansräkning has 100.00 on Inventarier, but the closing balances sum to 150.00
  document.closing[0].amount = 45000;
  document.vouchers.push_back(voucher("5", {{"1221", 5000}, {"1930", -5000}}));
  auto const statement = cratchit::ar::build_statement(layout(), {Balances{{"1221", 40000}, {"1229", -30000}}});
  auto const changed = build_notes(statement, document);
  ASSERT_EQ(changed.errors.size(), 1u);
  EXPECT_EQ(changed.errors.front(),
            "Not 3 Inventarier: Redovisat värde 150.00 differs from Inventarier 100.00 in the balansräkning");
}

TEST(ArNotes, UnknownSectionIsAnError) {
  auto const lines = cratchit::ar::parse_layout("Nivå;Typ;BAS Range;ÅR\n1;Rubrik;;Balansräkning\n3;Post;1220-1229;Inventarier\n"
                                                "1;Rubrik;;Noter\n2;Rubrik;;Not 3 Inventarier\n3;Post;1220-1229;Värden\n"
                                                "2;Summa;;Redovisat värde\n");
  ASSERT_TRUE(lines);
  auto const statement = cratchit::ar::build_statement(*lines, {Balances{{"1221", 100}}});
  auto const notes = build_notes(statement, {});
  ASSERT_EQ(notes.errors.size(), 2u);
  EXPECT_EQ(notes.errors.front(),
            "Not 3 Inventarier: unknown section Värden; expected Anskaffningsvärden, Avskrivningar or Nedskrivningar");
}

TEST(ArNotes, NoteWithoutAmountsIsNotShown) {
  auto const statement = cratchit::ar::build_statement(layout(), {Balances{{"1930", 100}}});
  EXPECT_TRUE(build_notes(statement, {}).notes.empty());
  // Not 1 is always there
  auto const text = cratchit::ar::format_notes({}, {{0, 2023y / May / 1, 2024y / April / 30}}, {});
  EXPECT_TRUE(text.starts_with("Noter\n=====\n\nNot 1 Redovisningsprinciper\n")) << text;
  EXPECT_EQ(text.find("Not 3"), std::string::npos);
}

namespace {

class ArNotesAgainstPdf : public testing::TestWithParam<std::pair<char const*, char const*>> {};

} // namespace

TEST_P(ArNotesAgainstPdf, AmountsMatchWithinAKrona) {
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
  auto const notes = build_notes(statement, document);
  EXPECT_TRUE(notes.errors.empty());
  ASSERT_EQ(notes.notes.size(), 1u);
  auto const& note = notes.notes.front();

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
  // Exactly the expected rows, in order: the sections' rows, then the sum
  std::vector<std::pair<std::string, Amount>> shown;
  for (auto const& section : note.sections) {
    for (auto const& row : section.rows) {
      shown.emplace_back(row.text, row.amount);
    }
  }
  shown.emplace_back(note.sum_text, note.sum);
  ASSERT_EQ(rows->size() - 1, shown.size());
  for (std::size_t index = 1; index < rows->size(); ++index) {
    auto const& row = (*rows)[index];
    EXPECT_EQ(row[0], shown[index - 1].first);
    cratchit::Kronor expected = 0;
    std::from_chars(row[1].data(), row[1].data() + row[1].size(), expected);
    auto const kronor = cratchit::to_kronor(shown[index - 1].second);
    EXPECT_LE(std::abs(kronor - expected), 1) << row[0] << ": " << kronor << " against " << expected;
  }
}

INSTANTIATE_TEST_SUITE_P(Filed, ArNotesAgainstPdf,
                         testing::Values(std::pair{"TheITfiedAB20230503_152345.se", "220501-230430_not_3.csv"},
                                         std::pair{"TheITfiedAB20250817_172409.se", "240501-250430_not_3.csv"}));
