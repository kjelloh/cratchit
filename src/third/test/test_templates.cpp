#include "core/books/books.hpp"
#include "core/templates.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <utility>

using namespace cratchit;
using namespace std::chrono;

namespace {

Date on(int d, unsigned m, int y = 2026) {
  return Date{std::chrono::year{y}, month{m}, std::chrono::day{static_cast<unsigned>(d)}};
}

books::Posting row(std::string account, Amount amount, std::string text = {}) {
  books::Posting p;
  p.account = std::move(account);
  p.amount = amount;
  p.text = std::move(text);
  return p;
}

books::Voucher voucher(std::string series, std::string number, std::string heading, Amount amount, Date date,
                       books::Postings postings) {
  return {std::move(series), std::move(number), {std::move(heading), amount, date}, date, "", std::move(postings)};
}

books::Books books_with(std::vector<books::Voucher> vouchers) {
  books::Books books;
  for (auto const& [number, name] : {std::pair{"1920", "PlusGiro"}, std::pair{"2440", "Leverantörsskulder"},
                                      std::pair{"2641", "Debiterad ingående moms"}, std::pair{"6212", "Mobiltelefon"},
                                      std::pair{"6570", "Bankkostnader"}}) {
    books.accounts.push_back({number, name, "", "", ""});
  }
  books::FiscalYear year{on(1, 5), on(30, 4, 2027)};
  year.vouchers = std::move(vouchers);
  books.years.push_back(std::move(year));
  return books;
}

books::Voucher const fee_may = voucher("A", "2", "AVGIFTER NORDEA", 185, on(6, 5), {row("1920", -185), row("6570", 185)});
books::Voucher const fee_april =
    voucher("A", "50", "Account:NORDEA Text:AVGIFTER NORDEA", 740, on(7, 4), {row("1920", -740), row("6570", 740)});
books::Voucher const telia = voucher("D", "3", "Faktura: Telia", 235900, on(18, 5),
                                     {row("2440", -235900), row("6212", 188700), row("2641", 47175), row("3740", 25)});
books::Voucher const interest = voucher("A", "10", "Skattekonto: Intäktsränta", 100, on(2, 5), {row("1920", 100), row("8310", -100)});

} // namespace

TEST(FindTemplates, AnAccountEventFindsVouchersWithItsAccountAndSign) {
  auto const books = books_with({fee_april, fee_may, telia, interest});
  books::Draft const draft{{"", 0, on(3, 7)}, on(17, 11), {row("1920", -370, "AVGIFTER NORDEA")}};
  auto const found = find_templates(books, draft);
  // 1920 credit: the fees, not the interest (1920 debit) nor Telia (no 1920); one per pattern, the latest
  ASSERT_EQ(found.size(), 1u);
  EXPECT_EQ(found[0].voucher.number, "2");
  EXPECT_EQ(found[0].words, (std::vector<std::string>{"avgifter", "nordea"}));
}

TEST(FindTemplates, AnEventWithoutPostingsFindsVouchersByWords) {
  auto const books = books_with({fee_may, telia});
  books::Draft const draft{{"Telia faktura oktober", 111800, on(22, 10)}, on(17, 11), {}};
  auto const found = find_templates(books, draft);
  ASSERT_EQ(found.size(), 1u);
  EXPECT_EQ(found[0].voucher.number, "3");
  // Nothing shared: nothing found
  books::Draft const other{{"Kontorsstol", 199900, on(22, 10)}, on(17, 11), {}};
  EXPECT_TRUE(find_templates(books, other).empty());
}

TEST(ApplyTemplate, ScaledToTheDraftTheLastRowBalancesTheSeriesComesAlong) {
  books::Draft const draft{{"Telia faktura oktober", 111800, on(22, 10)}, on(17, 11), {}};
  auto const applied = apply_template(draft, telia, books_with({}).accounts);
  // 3740 is not in the chart: left out
  EXPECT_EQ(applied.left_out, (std::vector<std::string>{"3740"}));
  auto const& result = applied.draft;
  EXPECT_EQ(result.event.heading, "Telia faktura oktober"); // not the template's text
  EXPECT_EQ(result.series, "D");
  ASSERT_EQ(result.postings.size(), 3u);
  EXPECT_EQ(result.postings[0].account, "2440");
  EXPECT_EQ(result.postings[0].amount, -111800);
  EXPECT_EQ(books::sum(result.postings), 0);
  EXPECT_EQ(books::stage(result), books::DraftStage::ready);
}

TEST(ApplyTemplate, AnAccountEventKeepsItsPostingAndGetsTheOthers) {
  books::Draft const draft{{"", 0, on(3, 7)}, on(17, 11), {row("1920", -370, "AVGIFTER NORDEA")}};
  auto const applied = apply_template(draft, fee_may, books_with({}).accounts);
  auto const& result = applied.draft;
  EXPECT_EQ(result.event.amount, 370); // what its parts amount to
  ASSERT_EQ(result.postings.size(), 2u);
  EXPECT_EQ(result.postings[0], draft.postings[0]);
  EXPECT_EQ(result.postings[1].account, "6570");
  EXPECT_EQ(result.postings[1].amount, 370);
  EXPECT_EQ(result.series, "A");
}

TEST(ApplyTemplate, ACreditNoteIsFoundAndUsedTurned) {
  auto const books = books_with({telia});
  books::Draft const draft{{"Telia kreditfaktura", -124500, on(20, 11)}, on(18, 11), {}};
  auto const found = find_templates(books, draft);
  ASSERT_EQ(found.size(), 1u);
  auto const result = apply_template(draft, found[0].voucher, books.accounts).draft;
  EXPECT_EQ(result.postings[0].account, "2440");
  EXPECT_EQ(result.postings[0].amount, 124500); // debit: the supplier owes us
  EXPECT_EQ(books::sum(result.postings), 0);
  EXPECT_EQ(books::stage(result), books::DraftStage::ready);
  // A credit note's account event (money in on 1920) finds the payments turned
  auto const paid = voucher("E", "1", "Betalning Telia", 1000, on(1, 6), {row("1920", -1000), row("2440", 1000)});
  books::Draft const refund{{"Återbetalning Telia", -1000, on(2, 12)}, on(18, 11), {row("1920", 1000)}};
  auto const refunds = find_templates(books_with({paid}), refund);
  ASSERT_EQ(refunds.size(), 1u);
  EXPECT_EQ(apply_template(refund, paid, books.accounts).draft.postings[1].amount, -1000);
}
