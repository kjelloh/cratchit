#include "core/books/books.hpp"
#include "core/inventory.hpp"
#include "core/sie_project.hpp"
#include "core/statement_drafts.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using namespace cratchit;
using namespace std::chrono;

namespace {

// A project from an SIE file for 2025: A1 a sale of 500 into the bank on
// 2025-03-01, A2 a bank fee of 10 on 2025-03-05
class StatementDraftsTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    folder = fs::temp_directory_path() / (std::string("cratchit_statement_test_") + info->name());
    fs::remove_all(folder);
    fs::create_directories(folder / "bank");
    std::ofstream(folder / "test.se", std::ios::binary)
        << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
           "#KONTO 1930 \"Bank\"\r\n#KONTO 3010 \"Sales\"\r\n#KONTO 6570 \"Bank costs\"\r\n"
           "#VER A 1 20250301 \"Sale\"\r\n{\r\n#TRANS 1930 {} 500\r\n#TRANS 3010 {} -500\r\n}\r\n"
           "#VER A 2 20250305 \"Fee\"\r\n{\r\n#TRANS 6570 {} 10\r\n#TRANS 1930 {} -10\r\n}\r\n";
    auto plan = plan_project_from_sie(folder / "test.se");
    ASSERT_TRUE(plan) << plan.error();
    ASSERT_TRUE(create_project_from_sie(*plan, "2026-11-09"));
    // Nordea-like: newest first, a saldo on each line
    statement_file = folder / "bank" / "nordea.csv";
    std::ofstream(statement_file, std::ios::binary)
        << "\xEF\xBB\xBF"
           "Datum;Belopp;Text;Saldo\n"
           "2026/01/02;-1,85;AVGIFTER;488,15\n"
           "2025/03/10;-10,00;AVGIFTER;490,00\n"
           "2025/03/05;-10,00;AVGIFTER;500,00\n"
           "2025/03/01;500,00;Kund;510,00\n"
           "2024/12/31;10,00;Ränta;10,00\n";
  }
  void TearDown() override { fs::remove_all(folder); }

  books::FiscalYear year_2025() const {
    auto const books = books::read_books(folder);
    EXPECT_TRUE(books) << books.error();
    auto const* year = books->year_starting(Date{2025y, January, 1d});
    EXPECT_NE(year, nullptr);
    return year != nullptr ? *year : books::FiscalYear{};
  }

  fs::path folder;
  fs::path statement_file;
};

} // namespace

TEST_F(StatementDraftsTest, ReadsTheFileOldestFirst) {
  auto const statement = read_statement_file(statement_file);
  ASSERT_TRUE(statement) << statement.error();
  ASSERT_EQ(statement->document.entries.size(), 5u);
  EXPECT_EQ(statement->document.entries.front().date, (Date{2024y, December, 31d}));
  EXPECT_EQ(statement->document.encoding, "UTF-8 med BOM");
  auto const missing = read_statement_file(folder / "bank" / "saknas.csv");
  ASSERT_FALSE(missing);
}

TEST_F(StatementDraftsTest, GuessesTheAccountWithTheMostMatchingPostings) {
  auto const books = books::read_books(folder);
  ASSERT_TRUE(books);
  auto const statement = read_statement_file(statement_file);
  ASSERT_TRUE(statement);
  auto const guess = guess_statement_account(*books, statement->document);
  ASSERT_TRUE(guess);
  EXPECT_EQ(guess->account, "1930");
  EXPECT_EQ(guess->matches, 2u); // 2025-03-01 500 and 2025-03-05 -10
}

TEST_F(StatementDraftsTest, PlansNewBookedAndOutside) {
  auto const statement = read_statement_file(statement_file);
  ASSERT_TRUE(statement);
  auto const plan = plan_statement_drafts(year_2025(), statement->document, "1930");
  EXPECT_EQ(plan.account, "1930");
  EXPECT_EQ(plan.outside.size(), 2u); // 2024-12-31 and 2026-01-02
  EXPECT_EQ(plan.booked.size(), 2u);
  EXPECT_TRUE(plan.drafted.empty());
  ASSERT_EQ(plan.added.size(), 1u); // the second fee of -10: A2 matches one of them only
  EXPECT_EQ(plan.added.front().date, (Date{2025y, March, 10d}));
}

TEST_F(StatementDraftsTest, AddsDraftsWithSourceAndRecordsTheFile) {
  auto const statement = read_statement_file(statement_file);
  ASSERT_TRUE(statement);
  auto const plan = plan_statement_drafts(year_2025(), statement->document, "1930");
  auto const added = add_statement_drafts(folder, statement_file, plan, Date{2025y, January, 1d}, Date{2026y, November, 17d});
  ASSERT_TRUE(added) << added.error();
  EXPECT_EQ(*added, 1u);
  auto const year = year_2025();
  ASSERT_EQ(year.drafts.size(), 1u);
  auto const& draft = year.drafts.front();
  EXPECT_EQ(draft.event.heading, "");
  EXPECT_EQ(draft.event.amount, 0);
  EXPECT_EQ(draft.event.date, (Date{2025y, March, 10d}));
  EXPECT_EQ(draft.noted, (Date{2026y, November, 17d}));
  EXPECT_EQ(books::stage(draft), books::DraftStage::account_event);
  ASSERT_EQ(draft.postings.size(), 1u);
  EXPECT_EQ(draft.postings.front().account, "1930");
  EXPECT_EQ(draft.postings.front().amount, -1000);
  EXPECT_EQ(draft.postings.front().text, "AVGIFTER");
  EXPECT_EQ(draft.postings.front().source, "bank/nordea.csv:3");
  // The file stays where it is, and the year's inventory has it
  EXPECT_TRUE(fs::exists(statement_file));
  auto const inventory = read_inventory(folder, Date{2025y, January, 1d});
  ASSERT_TRUE(inventory) << inventory.error();
  auto const found = std::ranges::find_if(*inventory, [](InventoryEntry const& e) { return e.kind == "kontoutdrag"; });
  ASSERT_NE(found, inventory->end());
  EXPECT_EQ(found->path, "bank/nordea.csv");
  EXPECT_EQ(found->use, "utkast");
  EXPECT_EQ(found->year_start, "2025-01-01");
  // Read again, the draft is known and nothing is new
  auto const again = plan_statement_drafts(year_2025(), statement->document, "1930");
  EXPECT_TRUE(again.added.empty());
  EXPECT_EQ(again.drafted.size(), 1u);
  EXPECT_EQ(again.booked.size(), 2u);
}
