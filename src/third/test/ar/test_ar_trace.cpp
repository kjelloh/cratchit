#include "core/ar/equity.hpp"
#include "core/ar/notes.hpp"
#include "core/ar/text.hpp"
#include "core/ar/trace.hpp"
#include "core/sie/file.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

using cratchit::Amount;
using cratchit::Balances;
using cratchit::ar::trace_statement;
using cratchit::sie::Transaction;
using namespace std::chrono;

namespace {

std::vector<cratchit::ar::Line> small_layout() {
  auto const lines = cratchit::ar::parse_layout("Nivå;Typ;BAS Range;ÅR\n"
                                                "1;Rubrik;;Resultaträkning\n"
                                                "3;Post;3000-3999;Nettoomsättning\n"
                                                "1;Rubrik;;Balansräkning\n"
                                                "3;Post;1900-1999;Kassa och bank\n"
                                                "3;Post;2610-2659;Övriga skulder\n");
  EXPECT_TRUE(lines);
  return *lines;
}

cratchit::sie::Voucher voucher(std::string number, cratchit::Date date, std::vector<std::pair<std::string, Amount>> const& postings,
                               Transaction::Kind kind = Transaction::Kind::normal) {
  auto text = "text " + number;
  cratchit::sie::Voucher out{"A", std::move(number), date, std::move(text), std::nullopt, "", {}, 0};
  for (auto const& [account, amount] : postings) {
    Transaction transaction;
    transaction.kind = kind;
    transaction.account = account;
    transaction.amount = amount;
    out.transactions.push_back(transaction);
  }
  return out;
}

} // namespace

TEST(ArTrace, AccountsAndVouchersByDate) {
  cratchit::sie::Document document;
  document.accounts = {{"1930", "Företagskonto", "", "", ""}};
  document.opening = {{0, "1930", 10000, ""}, {0, "2650", 4280, ""}};
  document.closing = {{0, "1930", 19280, ""}, {0, "2650", 0, ""}, {-1, "1930", 10000, ""}, {-1, "2650", 4280, ""}};
  document.results = {{0, "3001", -5000, ""}};
  document.vouchers = {
      voucher("2", 2023y / June / 1, {{"1930", 5000}, {"3001", -5000}}),
      voucher("1", 2023y / May / 16, {{"1930", 4280}, {"2650", -4280}}),
      voucher("3", 2023y / June / 2, {{"1930", 99900}, {"3001", -99900}}, Transaction::Kind::removed),
  };
  Balances const year0{{"1930", 19280}, {"2650", 0}, {"3001", -5000}};
  Balances const year1{{"1930", 10000}, {"2650", 4280}};
  auto const statement = cratchit::ar::build_statement(small_layout(), {year0, year1});
  auto const traced = trace_statement(statement, document);
  ASSERT_EQ(traced.size(), 3u);
  EXPECT_EQ(traced[0].line->text, "Nettoomsättning");
  ASSERT_EQ(traced[0].accounts.size(), 1u);
  EXPECT_FALSE(traced[0].accounts[0].opening);
  ASSERT_EQ(traced[0].accounts[0].postings.size(), 1u); // the removed transaction does not count

  auto const& bank = traced[1].accounts.at(0);
  EXPECT_EQ(bank.name, "Företagskonto");
  EXPECT_EQ(bank.amounts, (std::vector<Amount>{19280, 10000}));
  EXPECT_EQ(bank.opening, 10000);
  ASSERT_EQ(bank.postings.size(), 2u);
  EXPECT_EQ(bank.postings[0].voucher->number, "1"); // by date
  EXPECT_EQ(bank.postings[1].voucher->number, "2");

  // Zero at both year ends, but it has an opening balance and a posting in year 0
  auto const& vat = traced[2].accounts.at(0);
  EXPECT_EQ(vat.account, "2650");
  EXPECT_EQ(vat.amounts, (std::vector<Amount>{0, 4280}));
  EXPECT_EQ(vat.postings.size(), 1u);

  auto const text = cratchit::ar::format_trace({}, {}, {}, statement, {}, document, {{0, 2023y / May / 1, 2024y / April / 30},
                                                                          {-1, 2022y / May / 1, 2023y / April / 30}});
  EXPECT_NE(text.find("\nKassa och bank                                                  193 kr        100 kr\n"
                      "  1930 Företagskonto                                            192.80        100.00\n"
                      "    Ingående balans 2023-05-01                                  100.00\n"
                      "    A 1 2023-05-16 text 1                                        42.80\n"
                      "    A 2 2023-06-01 text 2                                        50.00\n\n"),
            std::string::npos)
      << text;
}

namespace {

class ArTraceOfFile : public testing::TestWithParam<char const*> {};

} // namespace

// The trace explains every amount: a line is its accounts, an account its opening and vouchers
TEST_P(ArTraceOfFile, AddsUp) {
  auto const sie = std::filesystem::path(CRATCHIT_AR_SIE_FOLDER) / GetParam();
  if (!std::filesystem::exists(sie)) {
    GTEST_SKIP() << "No " << sie;
  }
  auto const lexed = cratchit::sie::lex_file(sie);
  ASSERT_TRUE(lexed);
  auto const document = cratchit::sie::parse_document(lexed->lines).document;
  auto const layout = cratchit::ar::parse_layout(cratchit::ar::layout_text());
  ASSERT_TRUE(layout);
  auto const statement = cratchit::ar::build_statement(
      *layout, {cratchit::year_balances(document, 0), cratchit::year_balances(document, -1)});

  auto const traced = trace_statement(statement, document);
  std::size_t shown = 0;
  for (auto const& line : statement.lines) {
    shown += line.kind == cratchit::ar::LineKind::item && line.shown && !line.in_notes;
  }
  EXPECT_EQ(traced.size(), shown);
  for (auto const& line : traced) {
    std::vector<Amount> sums(2, 0);
    for (auto const& account : line.accounts) {
      for (std::size_t year = 0; year < 2; ++year) {
        sums[year] += account.account.starts_with('1') ? account.amounts[year] : -account.amounts[year];
      }
      Amount explained = account.opening.value_or(0);
      for (auto const& posting : account.postings) {
        explained += posting.amount;
      }
      EXPECT_EQ(explained, account.amounts[0]) << line.line->text << " " << account.account;
    }
    EXPECT_EQ(sums, line.line->exact) << line.line->text;
  }

  auto const equity = cratchit::ar::equity_changes(*layout, document);
  for (auto const& row : equity.rows) {
    Amount explained = 0;
    for (auto const& [account, amount] : row.accounts) {
      explained -= amount;
    }
    for (auto const* voucher : row.vouchers) {
      for (auto const& transaction : voucher->transactions) {
        bool const is_equity = std::ranges::any_of(
            equity.column_accounts, [&](cratchit::ar::AccountSet const& set) { return set.contains(transaction.account); });
        if (is_equity && transaction.kind != Transaction::Kind::removed) {
          explained -= transaction.amount;
        }
      }
    }
    EXPECT_EQ(explained, row.amounts.back()) << row.text;
  }

  auto const notes = cratchit::ar::build_notes(statement, document);
  EXPECT_FALSE(notes.notes.empty());
  for (auto const& note : notes.notes) {
    for (auto const& section : note.sections) {
      for (auto const& row : section.rows) {
        Amount explained = 0;
        for (auto const& [account, amount] : row.accounts) {
          explained += amount;
        }
        for (auto const* voucher : row.vouchers) {
          for (auto const& transaction : voucher->transactions) {
            if (transaction.kind != Transaction::Kind::removed && section.accounts.contains(transaction.account)) {
              explained += transaction.amount;
            }
          }
        }
        EXPECT_EQ(explained, row.amount) << note.title << " " << row.text;
      }
    }
  }
}

INSTANTIATE_TEST_SUITE_P(Filed, ArTraceOfFile,
                         testing::Values("TheITfiedAB20230503_152345.se", "TheITfiedAB20240727_213539.se",
                                         "TheITfiedAB20250817_172409.se"));
