#include "core/books/books.hpp"

#include <gtest/gtest.h>

#include <algorithm>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
using namespace std::chrono;
using cratchit::books::Books;
using cratchit::books::FiscalYear;
using cratchit::books::Posting;
using cratchit::books::Voucher;

namespace {

cratchit::Date date(int y, unsigned m, unsigned d) {
  return year_month_day(year{y}, month{m}, day{d});
}

Voucher voucher(std::string number, cratchit::Date when, std::string account, cratchit::Amount amount,
                std::string other = "3010") {
  return {"A", std::move(number), {"Text, with \"quotes\"", amount < 0 ? -amount : amount, when}, date(2026, 1, 10), "", {
      {Posting::Kind::normal, std::move(account), {}, amount, {}, "", "", ""},
      {Posting::Kind::normal, std::move(other), {{"1", "100"}, {"6", "P 1"}}, -amount, when, "rad", "2", ""},
  }};
}

Books sample() {
  Books books;
  books.chart_type = "EUBAS97";
  books.company_id = "503072";
  books.address = {"Kim", "Street 1", "17141 Solna", "070"};
  books.accounts = {{"1930", "Bank", "", "7281", ""}, {"2099", "Result", "", "", ""},
                    {"3010", "Sales", "", "", ""},   {"6570", "Bank costs", "K", "", ""}};
  books.dimensions = {{"1", "Kostnadsställe", ""}};
  books.objects = {{"1", "100", "Sälj"}};
  FiscalYear y2025{date(2025, 1, 1), date(2025, 12, 31), {{"1930", 100000}, {"2099", -100000}}, {}};
  y2025.vouchers.push_back(voucher("1", date(2025, 2, 1), "1930", 50000));
  y2025.vouchers.push_back(voucher("2", date(2025, 3, 1), "6570", 1000, "1930"));
  y2025.vouchers.back().postings.push_back({Posting::Kind::removed, "6110", {}, 999, {}, "", "", "Kim"});
  FiscalYear y2026{date(2026, 1, 1), date(2026, 12, 31), {{"1930", 1}}, {}};
  y2026.vouchers.push_back(voucher("1", date(2026, 2, 1), "1930", 700));
  books.years = {y2025, y2026};
  return books;
}

std::string read_file(fs::path const& path) {
  std::ifstream in(path);
  std::ostringstream text;
  text << in.rdbuf();
  return text.str();
}

class BooksFiles : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_books_") + info->name());
    fs::remove_all(root);
    fs::create_directories(root);
  }
  void TearDown() override { fs::remove_all(root); }

  fs::path root;
};

} // namespace

TEST(Postings, SumsDoNotCountRemovedPostings) {
  cratchit::books::Postings const postings{
      {.account = "6570", .amount = 1000},
      {.account = "1930", .amount = -1000},
      {.kind = Posting::Kind::removed, .account = "6110", .amount = 999},
  };
  EXPECT_EQ(cratchit::books::debit(postings), 1000);
  EXPECT_EQ(cratchit::books::credit(postings), 1000);
  EXPECT_EQ(cratchit::books::sum(postings), 0);
}

TEST(Balances, CarriedFromTheYearBeforeOnlyForBalanceAccounts) {
  auto const all = cratchit::books::balances(sample());
  ASSERT_EQ(all.size(), 2u);
  EXPECT_FALSE(all[0].carried);
  EXPECT_EQ(all[0].opening.at("1930"), 100000);
  EXPECT_EQ(all[0].change.at("1930"), 50000 - 1000);
  EXPECT_EQ(all[0].change.at("3010"), -50000);
  EXPECT_EQ(all[0].change.count("6110"), 0u); // removed rows do not count
  EXPECT_EQ(all[0].closing.at("1930"), 149000);
  EXPECT_TRUE(all[1].carried);
  EXPECT_EQ(all[1].opening.at("1930"), 149000); // not the reported 0.01
  EXPECT_EQ(all[1].opening.at("2099"), -100000);
  EXPECT_EQ(all[1].opening.count("3010"), 0u); // result accounts open at zero
  EXPECT_EQ(all[1].closing.at("1930"), 149700);
}

TEST(Balances, AGapMeansTheReportedOpeningBalances) {
  auto books = sample();
  books.years[1].start = date(2026, 1, 2);
  auto const all = cratchit::books::balances(books);
  EXPECT_FALSE(all[1].carried);
  EXPECT_EQ(all[1].opening.at("1930"), 1);
}

TEST_F(BooksFiles, NoBooksReadsAsEmpty) {
  auto const books = cratchit::books::read_books(root);
  ASSERT_TRUE(books.has_value()) << books.error();
  EXPECT_TRUE(books->years.empty());
  EXPECT_TRUE(books->accounts.empty());
}

TEST_F(BooksFiles, WriteThenReadGivesTheSameBooks) {
  auto const books = sample();
  auto const written = cratchit::books::write_books(root, books);
  ASSERT_TRUE(written.has_value()) << written.error();
  EXPECT_EQ(written->size(), 11u); // with series.csv
  EXPECT_TRUE(written->front().created);
  EXPECT_EQ(written->front().file, fs::path(".cratchit") / "books" / "books.txt");
  auto const back = cratchit::books::read_books(root);
  ASSERT_TRUE(back.has_value()) << back.error();
  EXPECT_EQ(back->chart_type, books.chart_type);
  EXPECT_EQ(back->company_id, books.company_id);
  EXPECT_EQ(back->address, books.address);
  EXPECT_EQ(back->accounts, books.accounts);
  EXPECT_EQ(back->dimensions, books.dimensions);
  EXPECT_EQ(back->objects, books.objects);
  // The sample names no series: each one its vouchers use is listed, unnamed
  for (auto const& year : books.years) {
    for (auto const& voucher : year.vouchers) {
      EXPECT_NE(std::ranges::find(back->series, voucher.series, &cratchit::books::Series::series), back->series.end());
    }
  }
  ASSERT_EQ(back->years.size(), 2u);
  for (std::size_t i = 0; i < 2; ++i) {
    EXPECT_EQ(back->years[i].start, books.years[i].start);
    EXPECT_EQ(back->years[i].end, books.years[i].end);
    EXPECT_EQ(back->years[i].reported_opening, books.years[i].reported_opening);
    EXPECT_EQ(back->years[i].vouchers, books.years[i].vouchers);
  }
}

TEST_F(BooksFiles, FileFormats) {
  ASSERT_TRUE(cratchit::books::write_books(root, sample()).has_value());
  auto const folder = root / ".cratchit" / "books";
  EXPECT_EQ(read_file(folder / "books.txt"), "chart_type = EUBAS97\ncompany_id = 503072\naddress.contact = Kim\n"
                                             "address.street = Street 1\naddress.postal = 17141 Solna\n"
                                             "address.phone = 070\nyear.2025-01-01 = 2025-12-31\n"
                                             "year.2026-01-01 = 2026-12-31\n");
  EXPECT_EQ(read_file(folder / "2025-01-01" / "opening.csv"), "account,amount\n1930,1000.00\n2099,-1000.00\n");
  EXPECT_EQ(read_file(folder / "2026-01-01" / "vouchers.csv"),
            "series,number,date,text,amount,registered,signature\n"
            "A,1,2026-02-01,\"Text, with \"\"quotes\"\"\",7.00,2026-01-10,\n");
  EXPECT_EQ(read_file(folder / "2026-01-01" / "postings.csv"),
            "series,number,kind,account,objects,amount,date,text,quantity,signature\n"
            "A,1,normal,1930,,7.00,,,,\n"
            "A,1,normal,3010,\"1,100,6,P 1\",-7.00,2026-02-01,rad,2,\n");
}

TEST_F(BooksFiles, OnlyChangedFilesAreWritten) {
  auto books = sample();
  ASSERT_TRUE(cratchit::books::write_books(root, books).has_value());
  auto const again = cratchit::books::write_books(root, books);
  ASSERT_TRUE(again.has_value());
  EXPECT_TRUE(again->empty());
  books.years[1].vouchers.push_back(voucher("2", date(2026, 3, 1), "1930", 1));
  auto const changed = cratchit::books::write_books(root, books);
  ASSERT_TRUE(changed.has_value());
  ASSERT_EQ(changed->size(), 2u);
  EXPECT_EQ(changed->at(0).file, fs::path(".cratchit") / "books" / "2026-01-01" / "vouchers.csv");
  EXPECT_FALSE(changed->at(0).created);
}

TEST_F(BooksFiles, MalformedFileIsAnError) {
  ASSERT_TRUE(cratchit::books::write_books(root, sample()).has_value());
  std::ofstream(root / ".cratchit" / "books" / "2025-01-01" / "opening.csv") << "account,amount\n1930,1.234\n";
  auto const books = cratchit::books::read_books(root);
  ASSERT_FALSE(books.has_value());
  EXPECT_NE(books.error().find("'1.234' is not an amount"), std::string::npos) << books.error();
}

TEST_F(BooksFiles, MissingYearFileIsAnError) {
  ASSERT_TRUE(cratchit::books::write_books(root, sample()).has_value());
  fs::remove(root / ".cratchit" / "books" / "2026-01-01" / "vouchers.csv");
  auto const books = cratchit::books::read_books(root);
  ASSERT_FALSE(books.has_value());
  EXPECT_NE(books.error().find("Cannot read"), std::string::npos) << books.error();
}

TEST_F(BooksFiles, ClosedYearsAreWrittenAndReadBack) {
  auto books = sample();
  books.years[0].closed = true;
  ASSERT_TRUE(cratchit::books::write_books(root, books).has_value());
  EXPECT_NE(read_file(root / ".cratchit" / "books" / "books.txt").find("closed.2025-01-01 = yes\n"), std::string::npos);
  auto const back = cratchit::books::read_books(root);
  ASSERT_TRUE(back.has_value()) << back.error();
  EXPECT_TRUE(back->years[0].closed);
  EXPECT_FALSE(back->years[1].closed);
}

TEST_F(BooksFiles, DraftsAreKeptOnlyWhileThereAreAny) {
  auto books = sample();
  ASSERT_TRUE(cratchit::books::write_books(root, books).has_value());
  auto const events = root / ".cratchit" / "books" / "2026-01-01" / "events.csv";
  EXPECT_FALSE(fs::exists(events));
  books.years[1].drafts.push_back({{"Telia, oktober", 111800, date(2026, 10, 20)}, date(2026, 10, 23)});
  auto const added = cratchit::books::write_books(root, books);
  ASSERT_TRUE(added.has_value()) << added.error();
  ASSERT_EQ(added->size(), 1u);
  EXPECT_EQ(added->at(0).file, fs::path(".cratchit") / "books" / "2026-01-01" / "events.csv");
  EXPECT_TRUE(added->at(0).created);
  EXPECT_EQ(read_file(events), "date,text,amount,noted,series\n2026-10-20,\"Telia, oktober\",1118.00,2026-10-23,\n");
  auto const back = cratchit::books::read_books(root);
  ASSERT_TRUE(back.has_value()) << back.error();
  EXPECT_EQ(back->years[1].drafts, books.years[1].drafts);
  EXPECT_TRUE(back->years[0].drafts.empty());
  books.years[1].drafts.clear();
  auto const removed = cratchit::books::write_books(root, books);
  ASSERT_TRUE(removed.has_value()) << removed.error();
  ASSERT_EQ(removed->size(), 1u);
  EXPECT_TRUE(removed->at(0).removed);
  EXPECT_FALSE(fs::exists(events));
}

TEST(Books, DraftsArePlacedByDateTiesInTheOrderNoted) {
  cratchit::books::FiscalYear year{date(2026, 1, 1), date(2026, 12, 31), {}, {}};
  EXPECT_EQ(year.place_draft({{"Telia", 111800, date(2026, 10, 20)}, date(2026, 10, 23)}), 0u);
  EXPECT_EQ(year.place_draft({{"Loopia", 130875, date(2026, 9, 28)}, date(2026, 10, 23)}), 0u);
  EXPECT_EQ(year.place_draft({{"Telia, rest", 1000, date(2026, 10, 20)}, date(2026, 10, 24)}), 2u);
  EXPECT_EQ(year.place_draft({{"Hyra", 500000, date(2026, 11, 1)}, date(2026, 10, 24)}), 3u);
  EXPECT_EQ(year.place_draft({{"Telia, mer", 2000, date(2026, 10, 20)}, date(2026, 10, 25)}), 3u);
  std::vector<std::string> headings;
  for (auto const& draft : year.drafts) {
    headings.push_back(draft.event.heading);
  }
  EXPECT_EQ(headings, (std::vector<std::string>{"Loopia", "Telia", "Telia, rest", "Telia, mer", "Hyra"}));
}

namespace {

using cratchit::books::Draft;
using cratchit::books::DraftStage;

Posting row(std::string account, cratchit::Amount amount) {
  return {Posting::Kind::normal, std::move(account), {}, amount, {}, "", "", ""};
}

} // namespace

TEST(DraftStage, AnEventWithoutPostingsIsAnEvent) {
  EXPECT_EQ(stage(Draft{{"Telia, oktober", 111800, date(2026, 10, 20)}, date(2026, 10, 23)}), DraftStage::event);
  EXPECT_EQ(stage(Draft{{"", 0, date(2026, 10, 20)}, date(2026, 10, 23)}), DraftStage::event);
}

TEST(DraftStage, OnePostingAloneIsAnAccountEvent) {
  Draft draft{{"", 0, date(2026, 10, 20)}, date(2026, 10, 23), {row("1930", -111800)}};
  EXPECT_EQ(stage(draft), DraftStage::account_event);
  // More than one part of the same amount: balanced, but no heading
  draft.postings.push_back(row("6212", 111800));
  EXPECT_EQ(stage(draft), DraftStage::same);
  // One posting with an event of its amount: the same
  draft.postings.pop_back();
  draft.event = {"Telia", 111800, date(2026, 10, 20)};
  EXPECT_EQ(stage(draft), DraftStage::same);
  // Parts of different amounts, not ready
  draft.event.amount = 100000;
  EXPECT_EQ(stage(draft), DraftStage::unbalanced);
}

TEST(DraftStage, AHeadingWithPostingsThatDoNotBalanceIsNotReady) {
  Draft draft{{"Telia, oktober", 111800, date(2026, 10, 20)}, date(2026, 10, 23), {row("1930", -111800)}};
  EXPECT_EQ(stage(draft), DraftStage::same);
  draft.postings.push_back(row("6212", 89440));
  EXPECT_EQ(stage(draft), DraftStage::unbalanced);
}

TEST(DraftStage, BalancedPostingsThatAddUpToTheAmountAreReady) {
  Draft draft{{"Telia, oktober", 111800, date(2026, 10, 20)}, date(2026, 10, 23),
              {row("6212", 89440), row("2641", 22360), row("1930", -111800)}};
  EXPECT_EQ(stage(draft), DraftStage::ready);
}

TEST(DraftStage, BalancedPostingsThatDoNotAddUpToTheAmountAreUnbalanced) {
  Draft draft{{"Telia, oktober", 100000, date(2026, 10, 20)}, date(2026, 10, 23),
              {row("6212", 111800), row("1930", -111800)}};
  EXPECT_EQ(stage(draft), DraftStage::unbalanced);
  draft.event.amount = 0;
  draft.postings = {row("6212", 0), row("1930", 0)};
  EXPECT_EQ(stage(draft), DraftStage::same); // nothing to book
}

TEST(DraftStage, APostingWithoutAnAccountIsNotReady) {
  Draft draft{{"Telia, oktober", 111800, date(2026, 10, 20)}, date(2026, 10, 23),
              {row("", 111800), row("1930", -111800)}};
  EXPECT_EQ(stage(draft), DraftStage::same);
}

TEST(DraftStage, RemovedPostingsDoNotCount) {
  Draft draft{{"Telia, oktober", 111800, date(2026, 10, 20)}, date(2026, 10, 23),
              {row("6212", 111800), row("1930", -111800), row("", 5000)}};
  draft.postings[2].kind = Posting::Kind::removed;
  EXPECT_EQ(stage(draft), DraftStage::ready);
  draft.postings[0].kind = Posting::Kind::removed;
  draft.postings[1].kind = Posting::Kind::removed;
  EXPECT_EQ(stage(draft), DraftStage::event);
}

TEST_F(BooksFiles, DraftsInNotedOrderAreReadByDateWithTheirPostings) {
  auto const books = sample();
  ASSERT_TRUE(cratchit::books::write_books(root, books).has_value());
  auto const year = root / ".cratchit" / "books" / "2026-01-01";
  std::ofstream(year / "events.csv") << "date,text,amount,noted\n"
                                        "2026-10-20,Telia,1118.00,2026-10-23\n"
                                        "2026-01-24,Till sparkonto,50000.00,2026-10-24\n"
                                        "2026-10-20,Telia rest,10.00,2026-10-24\n";
  std::ofstream(year / "event_postings.csv") << "event,account,amount,date,text,source\n"
                                                "2,1920,-50000.00,2026-01-24,PG TILL SPAR,s.csv:12\n"
                                                "3,1920,-10.00,2026-10-20,TELIA,s.csv:13\n";
  auto const back = cratchit::books::read_books(root);
  ASSERT_TRUE(back.has_value()) << back.error();
  auto const& drafts = back->years[1].drafts;
  ASSERT_EQ(drafts.size(), 3u);
  EXPECT_EQ(drafts[0].event.heading, "Till sparkonto");
  ASSERT_EQ(drafts[0].postings.size(), 1u);
  EXPECT_EQ(drafts[0].postings[0].amount, -5000000);
  EXPECT_EQ(drafts[1].event.heading, "Telia");
  EXPECT_TRUE(drafts[1].postings.empty());
  EXPECT_EQ(drafts[2].event.heading, "Telia rest");
  ASSERT_EQ(drafts[2].postings.size(), 1u);
  EXPECT_EQ(drafts[2].postings[0].amount, -1000);
  ASSERT_TRUE(cratchit::books::write_books(root, *back).has_value());
  // Read without series (as written before 2026-11-18), written with it
  EXPECT_EQ(read_file(year / "events.csv"), "date,text,amount,noted,series\n"
                                            "2026-01-24,Till sparkonto,50000.00,2026-10-24,\n"
                                            "2026-10-20,Telia,1118.00,2026-10-23,\n"
                                            "2026-10-20,Telia rest,10.00,2026-10-24,\n");
  EXPECT_EQ(read_file(year / "event_postings.csv"), "event,account,amount,date,text,source\n"
                                                    "1,1920,-50000.00,2026-01-24,PG TILL SPAR,s.csv:12\n"
                                                    "3,1920,-10.00,2026-10-20,TELIA,s.csv:13\n");
}

TEST_F(BooksFiles, TransactionsCsvIsReadAndBecomesPostingsCsv) {
  auto const books = sample();
  ASSERT_TRUE(cratchit::books::write_books(root, books).has_value());
  auto const year = root / ".cratchit" / "books" / "2026-01-01";
  fs::rename(year / "postings.csv", year / "transactions.csv");
  auto const back = cratchit::books::read_books(root);
  ASSERT_TRUE(back.has_value()) << back.error();
  EXPECT_EQ(back->years[1].vouchers, books.years[1].vouchers);
  auto const written = cratchit::books::write_books(root, *back);
  ASSERT_TRUE(written.has_value()) << written.error();
  ASSERT_EQ(written->size(), 2u);
  EXPECT_EQ(written->at(0).file, fs::path(".cratchit") / "books" / "2026-01-01" / "postings.csv");
  EXPECT_TRUE(written->at(0).created);
  EXPECT_EQ(written->at(1).file, fs::path(".cratchit") / "books" / "2026-01-01" / "transactions.csv");
  EXPECT_TRUE(written->at(1).removed);
  EXPECT_TRUE(fs::exists(year / "postings.csv"));
  EXPECT_FALSE(fs::exists(year / "transactions.csv"));
}

TEST_F(BooksFiles, DraftPostingsAreKeptOnlyWhileThereAreAny) {
  auto books = sample();
  books.years[1].place_draft({{"Telia", 111800, date(2026, 10, 20)}, date(2026, 10, 23)});
  books.years[1].place_draft({{"Till sparkonto", 5000000, date(2026, 1, 24)}, date(2026, 10, 24)});
  cratchit::books::Posting out;
  out.account = "1920";
  out.amount = -5000000;
  out.date = date(2026, 1, 24);
  out.text = "PG TILL SPAR";
  out.source = ".cratchit/statements/2026-10-24_1_pg.csv:12";
  auto in = out;
  in.account = "1930";
  in.amount = 5000000;
  in.source = ".cratchit/statements/2026-10-24_2_spar.csv:3";
  books.years[1].drafts[0].postings = {out, in};
  ASSERT_TRUE(cratchit::books::write_books(root, books).has_value());
  auto const file = root / ".cratchit" / "books" / "2026-01-01" / "event_postings.csv";
  EXPECT_EQ(read_file(file), "event,account,amount,date,text,source\n"
                             "1,1920,-50000.00,2026-01-24,PG TILL SPAR,.cratchit/statements/2026-10-24_1_pg.csv:12\n"
                             "1,1930,50000.00,2026-01-24,PG TILL SPAR,.cratchit/statements/2026-10-24_2_spar.csv:3\n");
  auto const back = cratchit::books::read_books(root);
  ASSERT_TRUE(back.has_value()) << back.error();
  EXPECT_EQ(back->years[1].drafts, books.years[1].drafts);
  books.years[1].drafts.erase(books.years[1].drafts.begin());
  auto const removed = cratchit::books::write_books(root, books);
  ASSERT_TRUE(removed.has_value()) << removed.error();
  EXPECT_FALSE(fs::exists(file));
  std::ofstream(file) << "event,account,amount,date,text,source\n3,1920,1.00,,,\n";
  auto const bad = cratchit::books::read_books(root);
  ASSERT_FALSE(bad.has_value());
  EXPECT_NE(bad.error().find("event 3 is not in events.csv"), std::string::npos) << bad.error();
}

TEST_F(BooksFiles, BooksOfOnlyAYearAreRead) {
  fs::create_directories(root / ".cratchit" / "books");
  std::ofstream(root / ".cratchit" / "books" / "books.txt") << "year.2026-01-01 = 2026-12-31\n";
  auto const books = cratchit::books::read_books(root);
  ASSERT_TRUE(books.has_value()) << books.error();
  EXPECT_TRUE(books->accounts.empty());
  ASSERT_EQ(books->years.size(), 1u);
  EXPECT_TRUE(books->years[0].vouchers.empty());
}

TEST(VoucherId, IsSeriesAndNumberAsOneWord) {
  EXPECT_EQ(cratchit::books::voucher_id("A", "12"), "A12");
  EXPECT_EQ(cratchit::books::voucher_id("D", "1"), "D1");
}

TEST(VoucherId, IsParsedFromOneWordOnly) {
  using Id = std::pair<std::string, std::string>;
  EXPECT_EQ(cratchit::books::parse_voucher_id("A12"), (Id{"A", "12"}));
  EXPECT_EQ(cratchit::books::parse_voucher_id("D1"), (Id{"D", "1"}));
  EXPECT_EQ(cratchit::books::parse_voucher_id("AB7"), (Id{"AB", "7"}));
  EXPECT_FALSE(cratchit::books::parse_voucher_id("A 12"));
  EXPECT_FALSE(cratchit::books::parse_voucher_id("12"));
  EXPECT_FALSE(cratchit::books::parse_voucher_id("A"));
  EXPECT_FALSE(cratchit::books::parse_voucher_id("A12B"));
  EXPECT_FALSE(cratchit::books::parse_voucher_id(""));
}

TEST_F(BooksFiles, BooksWithoutAListOfSeriesHaveFortnoxsSeries) {
  ASSERT_TRUE(cratchit::books::write_books(root, sample()));
  fs::remove(root / ".cratchit" / "books" / "series.csv");
  auto const books = cratchit::books::read_books(root);
  ASSERT_TRUE(books) << books.error();
  EXPECT_EQ(books->series_name("A"), "Redovisning");
  EXPECT_EQ(books->series_name("D"), "Leverantörsfakturor");
  EXPECT_EQ(books->series_name("N"), "Lager");
  EXPECT_EQ(books->series_name("Z"), "");
}

TEST_F(BooksFiles, SeriesAreReadAndWrittenWithTheirNames) {
  auto books = sample();
  books.series = {{"A", "Bank"}, {"X", "Egen serie"}};
  ASSERT_TRUE(cratchit::books::write_books(root, books));
  auto const back = cratchit::books::read_books(root);
  ASSERT_TRUE(back) << back.error();
  EXPECT_EQ(back->series_name("A"), "Bank");
  EXPECT_EQ(back->series_name("X"), "Egen serie");
}

TEST(ContinuedYear, YearsAfterTheLastAreAddedTwelveMonthsEach) {
  Books books;
  books.years.push_back(FiscalYear{date(2025, 5, 1), date(2026, 4, 30)});
  // In the books: that year
  EXPECT_EQ(cratchit::books::continued_year(books, date(2025, 5, 1)), &books.years.front());
  // Not the start of a year, or before the last: nothing added
  EXPECT_EQ(cratchit::books::continued_year(books, date(2026, 6, 1)), nullptr);
  EXPECT_EQ(cratchit::books::continued_year(books, date(2024, 5, 1)), nullptr);
  EXPECT_EQ(books.years.size(), 1u);
  // Two years on: the year between is added too
  auto const* year = cratchit::books::continued_year(books, date(2027, 5, 1));
  ASSERT_NE(year, nullptr);
  ASSERT_EQ(books.years.size(), 3u);
  EXPECT_EQ(books.years[1].start, date(2026, 5, 1));
  EXPECT_EQ(books.years[1].end, date(2027, 4, 30));
  EXPECT_EQ(year->start, date(2027, 5, 1));
  EXPECT_EQ(year->end, date(2028, 4, 30));
  EXPECT_FALSE(year->closed);
  EXPECT_TRUE(year->reported_opening.empty());
  // No year in the books: nothing to run on from
  Books none;
  EXPECT_EQ(cratchit::books::continued_year(none, date(2026, 5, 1)), nullptr);
}
