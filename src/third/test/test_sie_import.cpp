#include "core/sie_import.hpp"

#include "core/inventory.hpp"
#include "core/sha256.hpp"

#include "core/fiscal_year.hpp"
#include "core/path_value.hpp"
#include "core/sie_project.hpp"
#include "sie_test_file.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

using sie_test::balanced_balances;
using sie_test::sale_and_cost;
using sie_test::sie_file;

namespace {

// A project created from Test AB's 2025 file (vouchers A 1 and A 2), in a folder of its own, removed afterwards
class SieImportTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_sie_import_") + info->name());
    fs::remove_all(root);
    fs::create_directories(root);
    auto const plan = cratchit::plan_project_from_sie(write("start.se", sie_file(2025, balanced_balances(), sale_and_cost(2025))));
    ASSERT_TRUE(plan) << plan.error();
    auto created = cratchit::create_project_from_sie(*plan, "2026-11-09");
    ASSERT_TRUE(created) << created.error();
    project = *created;
  }
  void TearDown() override { fs::remove_all(root); }

  fs::path write(std::string const& name, std::string const& text) const {
    std::ofstream(root / name, std::ios::binary) << text;
    return root / name;
  }

  static std::string sale(int year, int number, std::string const& day) {
    return std::format("#VER A {1} {0}{2} \"Sale\"\r\n{{\r\n#TRANS 1930 {{}} 100\r\n#TRANS 3010 {{}} -100\r\n}}\r\n", year,
                       number, day);
  }

  fs::path root;
  cratchit::Project project;
};

} // namespace

TEST_F(SieImportTest, NewYearIsPlannedThenImportedAndTheFileRecordedNotMoved) {
  auto const file = write("next.se", sie_file(2026, "", sale(2026, 1, "0110")));
  auto const plan = cratchit::plan_sie_import(project, file);
  ASSERT_TRUE(plan) << plan.error();
  EXPECT_TRUE(plan->new_year);
  EXPECT_EQ(plan->import.vouchers_added, 1u);
  EXPECT_EQ(cratchit::list_fiscal_years(project)->size(), 1u); // nothing written by the plan
  auto const imported = cratchit::import_sie(project, *plan, *cratchit::parse_iso_date("2026-11-09"));
  ASSERT_TRUE(imported) << imported.error();
  EXPECT_EQ(cratchit::list_fiscal_years(project),
            (std::vector<cratchit::FiscalYear>{{"2026-01-01", "2026-12-31"}, {"2025-01-01", "2025-12-31"}}));
  // The file stays where it is, recorded in the inventory of its year
  EXPECT_TRUE(fs::exists(file));
  EXPECT_FALSE(fs::exists(project.folder / "importerade_sie_filer"));
  EXPECT_EQ(imported->entry.sha256, cratchit::sha256_of_file(file).value());
  EXPECT_EQ(imported->entry.use, "import");
  EXPECT_EQ(imported->entry.vouchers_added, "1");
  auto const inventory = cratchit::read_inventory(project.folder, cratchit::parse_iso_date("2026-01-01"));
  ASSERT_TRUE(inventory) << inventory.error();
  EXPECT_EQ(*inventory, (std::vector<cratchit::InventoryEntry>{imported->entry}));
}

TEST_F(SieImportTest, NewVouchersAreAddedToAYearInTheBooks) {
  auto const file = write("more.se", sie_file(2025, balanced_balances(), sale_and_cost(2025) + sale(2025, 3, "0401")));
  auto const plan = cratchit::plan_sie_import(project, file);
  ASSERT_TRUE(plan) << plan.error();
  EXPECT_FALSE(plan->new_year);
  EXPECT_EQ(plan->import.vouchers_added, 1u);
  EXPECT_EQ(plan->import.vouchers_known, 2u);
  ASSERT_TRUE(cratchit::import_sie(project, *plan, *cratchit::parse_iso_date("2026-11-09")));
  auto const books = cratchit::books::read_books(project.folder);
  ASSERT_TRUE(books) << books.error();
  EXPECT_EQ(books->years[0].vouchers.size(), 3u);
}

TEST_F(SieImportTest, NothingNewIsNotPossible) {
  auto const plan = cratchit::plan_sie_import(project, write("same.se", sie_file(2025, balanced_balances(), sale_and_cost(2025))));
  ASSERT_TRUE(plan) << plan.error();
  EXPECT_FALSE(plan->possible());
  EXPECT_EQ(plan->blockers, (std::vector<std::string>{"Allt i SIE-filen finns redan i bokföringen."}));
  ASSERT_EQ(plan->series.size(), 1u);
  EXPECT_EQ(plan->series[0].books_last, "2");
  EXPECT_EQ(plan->series[0].file_last, "2");
  EXPECT_EQ(plan->series[0].new_numbers, "");
  EXPECT_FALSE(cratchit::import_sie(project, *plan, *cratchit::parse_iso_date("2026-11-09")));
}

TEST_F(SieImportTest, RemovingAVoucherIsNotPossibleAndNamesIt) {
  // The file lacks A 2, which the books have: importing would demote it
  auto const plan = cratchit::plan_sie_import(project, write("less.se", sie_file(2025, "", sale(2025, 1, "0201"))));
  ASSERT_TRUE(plan) << plan.error();
  EXPECT_FALSE(plan->possible());
  // The file's A 1 is another sale (100, not 500), and it lacks A 2
  ASSERT_EQ(plan->in_the_way.size(), 2u);
  EXPECT_EQ(plan->in_the_way[0].number, "1");
  EXPECT_EQ(plan->in_the_way[0].why, "har annat innehåll i filen");
  EXPECT_EQ(plan->in_the_way[1].series, "A");
  EXPECT_EQ(plan->in_the_way[1].number, "2");
  EXPECT_EQ(plan->in_the_way[1].heading, "Bank cost");
  EXPECT_EQ(plan->in_the_way[1].why, "finns inte i filen");
  EXPECT_TRUE(plan->blockers.back().starts_with("Bokföringen har 2 verifikationer som filen skulle ta bort")) << plan->blockers.back();
  auto const books = cratchit::books::read_books(project.folder);
  ASSERT_TRUE(books);
  EXPECT_EQ(books->years[0].vouchers.size(), 2u);
}

TEST_F(SieImportTest, AnotherOrgnrIsNotPossible) {
  auto const plan = cratchit::plan_sie_import(
      project, write("other.se", sie_file(2026, "", sale(2026, 1, "0110"), "\"Test AB\"", "556000-0002")));
  ASSERT_TRUE(plan) << plan.error();
  EXPECT_FALSE(plan->possible());
  EXPECT_NE(plan->blockers.front().find("556000-0002"), std::string::npos) << plan->blockers.front();
}

TEST_F(SieImportTest, NewVouchersAreComparedSeriesBySeriesAsRanges) {
  auto const plan = cratchit::plan_sie_import(
      project, write("more.se", sie_file(2025, balanced_balances(),
                                         sale_and_cost(2025) + sale(2025, 3, "0401") + sale(2025, 4, "0402") +
                                             sale(2025, 5, "0403"))));
  ASSERT_TRUE(plan) << plan.error();
  EXPECT_TRUE(plan->possible());
  ASSERT_EQ(plan->series.size(), 1u);
  EXPECT_EQ(plan->series[0].series, "A");
  EXPECT_EQ(plan->series[0].books_last, "2");
  EXPECT_EQ(plan->series[0].file_last, "5");
  EXPECT_EQ(plan->series[0].new_numbers, "3–5");
}

TEST_F(SieImportTest, AGapInASeriesOfTheFileIsNotPossible) {
  // A 3 is missing between the books' A 2 and the file's A 4
  auto const plan = cratchit::plan_sie_import(
      project, write("gap.se", sie_file(2025, balanced_balances(), sale_and_cost(2025) + sale(2025, 4, "0401"))));
  ASSERT_TRUE(plan) << plan.error();
  EXPECT_FALSE(plan->possible());
  EXPECT_EQ(plan->blockers, (std::vector<std::string>{"Serie A i filen har en lucka: A3 saknas."}));
}

TEST_F(SieImportTest, ANewYearThatLeavesAGapInTheBooksIsRefused) {
  // The books have 2025; 2027 would leave 2026 out
  auto const plan = cratchit::plan_sie_import(project, write("2027.se", sie_file(2027, "", sale(2027, 1, "0110"))));
  ASSERT_FALSE(plan);
  EXPECT_EQ(plan.error(), "SIE-filens räkenskapsår 2027-01-01 – 2027-12-31 ansluter inte till bokföringen, som har "
                          "2025-01-01 – 2025-12-31. Importera först åren emellan, så att bokföringen inte får några luckor");
  // 2026 and 2024 join it
  EXPECT_TRUE(cratchit::plan_sie_import(project, write("2026.se", sie_file(2026, "", sale(2026, 1, "0110")))));
  EXPECT_TRUE(cratchit::plan_sie_import(project, write("2024.se", sie_file(2024, "", sale(2024, 1, "0110")))));
}

TEST_F(SieImportTest, NoSieFileIsRefused) {
  EXPECT_FALSE(cratchit::plan_sie_import(project, write("notes.se", "notes\n")));
  EXPECT_FALSE(cratchit::plan_sie_import(project, root / "gone.se"));
}

namespace {

// The 2025 file with A 2's rows (10 to bank costs) replaced by rows
std::string with_a2_rows(std::string const& rows) {
  auto file = sie_file(2025, balanced_balances(), sale_and_cost(2025));
  std::string const original = "#TRANS 6570 {} 10\r\n#TRANS 1930 {} -10\r\n";
  file.replace(file.find(original), original.size(), rows);
  return file;
}

// A 2 with both rows struck and new ones added for 12, as Fortnox writes a changed voucher
std::string const struck_and_added = "#BTRANS 6570 {} 10 \"\" \"\" \"\" \"Kjell\"\r\n"
                                     "#BTRANS 1930 {} -10 \"\" \"\" \"\" \"Kjell\"\r\n"
                                     "#RTRANS 6570 {} 12 \"\" \"\" \"\" \"Kjell\"\r\n#TRANS 6570 {} 12\r\n"
                                     "#RTRANS 1930 {} -12 \"\" \"\" \"\" \"Kjell\"\r\n#TRANS 1930 {} -12\r\n";

} // namespace

TEST_F(SieImportTest, StruckAndAddedRowsChangeAVoucher) {
  auto const plan = cratchit::plan_sie_import(project, write("changed.se", with_a2_rows(struck_and_added)));
  ASSERT_TRUE(plan) << plan.error();
  EXPECT_EQ(plan->import.vouchers_added, 0u);
  ASSERT_EQ(plan->changed.size(), 1u);
  EXPECT_EQ(cratchit::change_text(plan->changed[0]), "A2  2025-03-01  Bank cost: 2 rader strukna, 2 rader tillagda");
  ASSERT_TRUE(cratchit::import_sie(project, *plan, *cratchit::parse_iso_date("2026-11-09")));
  auto const books = cratchit::books::read_books(project.folder);
  ASSERT_TRUE(books) << books.error();
  EXPECT_EQ(books->years[0].vouchers[1].postings.size(), 4u);
}

TEST_F(SieImportTest, AnotherTextOfAVoucherIsRefused) {
  auto file = with_a2_rows(struck_and_added);
  file.replace(file.find("\"Bank cost\""), 11, "\"Bank fee\"");
  auto const plan = cratchit::plan_sie_import(project, write("text.se", file));
  ASSERT_TRUE(plan) << plan.error();
  EXPECT_EQ(plan->blockers, (std::vector<std::string>{"Verifikationen A2 har ett annat datum eller en annan text i "
                                                      "filen. cratchit tar bara emot strukna och tillagda rader i "
                                                      "befintliga verifikationer."}));
}

TEST(ChangeText, SaysWhatChanged) {
  EXPECT_EQ(cratchit::change_text({"A", "12", "2025-06-03", "Loopia", 1, 1}), "A12  2025-06-03  Loopia: 1 rad struken, 1 rad tillagd");
  EXPECT_EQ(cratchit::change_text({"A", "12", "2025-06-03", "Loopia", 0, 3}), "A12  2025-06-03  Loopia: 3 rader tillagda");
}

TEST_F(SieImportTest, WithAYearTheFileMustBeForThatYear) {
  auto const file = write("more.se", sie_file(2025, balanced_balances(), sale_and_cost(2025) + sale(2025, 3, "0401")));
  EXPECT_TRUE(cratchit::plan_sie_import(project, file, cratchit::FiscalYear{"2025-01-01", "2025-12-31"}));
  auto const other = cratchit::plan_sie_import(project, file, cratchit::FiscalYear{"2026-01-01", "2026-12-31"});
  ASSERT_FALSE(other);
  EXPECT_EQ(other.error(), "SIE-filen gäller räkenskapsåret 2025-01-01 – 2025-12-31, inte 2026");
}

namespace {

// Test AB's SIE file for the fiscal year start – end (YYYYMMDD) with one sale on day
std::string sie_for(std::string const& start, std::string const& end, std::string const& day) {
  return std::format("#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 {} {}\r\n"
                     "#KONTO 1930 \"Bank\"\r\n#KONTO 3010 \"Sales\"\r\n"
                     "#VER A 1 {} \"Sale\"\r\n{{\r\n#TRANS 1930 {{}} 100\r\n#TRANS 3010 {{}} -100\r\n}}\r\n",
                     start, end, day);
}

std::vector<cratchit::FiscalYear> listed_years(cratchit::Project const& project, std::string const& today) {
  std::vector<cratchit::FiscalYear> result;
  auto const listed = cratchit::list_years_until(project, today);
  EXPECT_TRUE(listed) << listed.error();
  if (!listed) return result;
  for (auto const& year : *listed) result.push_back(year.year);
  return result;
}

} // namespace

TEST_F(SieImportTest, TheYearBeforeTakesAnEarlierYear) {
  // The books have 2025; the year before, 2024, is listed to import into
  auto const plan = cratchit::plan_sie_import(project, write("2024.se", sie_for("20240101", "20241231", "20240301")),
                                              cratchit::FiscalYear{"2024-01-01", "2024-12-31"});
  ASSERT_TRUE(plan) << plan.error();
  EXPECT_FALSE(plan->expected);
  ASSERT_TRUE(cratchit::import_sie(project, *plan, *cratchit::parse_iso_date("2026-11-09")));
  EXPECT_EQ(listed_years(project, "2025-06-01"),
            (std::vector<cratchit::FiscalYear>{{"2025-01-01", "2025-12-31"}, {"2024-01-01", "2024-12-31"}, {"2023-01-01", "2023-12-31"}}));
}

TEST_F(SieImportTest, TheYearBeforeTakesAFirstYearOfAnotherLength) {
  // The company's first year: 18 months, ending the day before 2025
  auto const plan = cratchit::plan_sie_import(project, write("first.se", sie_for("20230701", "20241231", "20240301")),
                                              cratchit::FiscalYear{"2024-01-01", "2024-12-31"});
  ASSERT_TRUE(plan) << plan.error();
  ASSERT_TRUE(plan->expected);
  EXPECT_EQ(*plan->expected, (cratchit::FiscalYear{"2024-01-01", "2024-12-31"}));
  ASSERT_TRUE(cratchit::import_sie(project, *plan, *cratchit::parse_iso_date("2026-11-09")));
  EXPECT_EQ(listed_years(project, "2025-06-01"),
            (std::vector<cratchit::FiscalYear>{{"2025-01-01", "2025-12-31"}, {"2023-07-01", "2024-12-31"}, {"2022-07-01", "2023-06-30"}}));
}

TEST_F(SieImportTest, TheFirstYearAfterTakesAChangeOfFiscalYear) {
  // Omläggning: four months, then years from May
  auto const plan = cratchit::plan_sie_import(project, write("change.se", sie_for("20260101", "20260430", "20260201")),
                                              cratchit::FiscalYear{"2026-01-01", "2026-12-31"});
  ASSERT_TRUE(plan) << plan.error();
  ASSERT_TRUE(plan->expected);
  ASSERT_TRUE(cratchit::import_sie(project, *plan, *cratchit::parse_iso_date("2026-11-09")));
  EXPECT_EQ(listed_years(project, "2026-11-09"), (std::vector<cratchit::FiscalYear>{{"2026-05-01", "2027-04-30"},
                                                                                   {"2026-01-01", "2026-04-30"},
                                                                                   {"2025-01-01", "2025-12-31"},
                                                                                   {"2024-01-01", "2024-12-31"}}));
}

TEST_F(SieImportTest, AYearInAGapNeedsTheSameDates) {
  // 2027 is not the first year after 2025, so it takes no other dates
  auto const plan = cratchit::plan_sie_import(project, write("gap.se", sie_for("20270101", "20270630", "20270201")),
                                              cratchit::FiscalYear{"2027-01-01", "2027-12-31"});
  ASSERT_FALSE(plan);
  EXPECT_EQ(plan.error(), "SIE-filen gäller räkenskapsåret 2027-01-01 – 2027-06-30, inte 2027");
}

TEST_F(SieImportTest, AnAdjoiningYearOfMoreThanEighteenMonthsIsRefused) {
  auto const plan = cratchit::plan_sie_import(project, write("long.se", sie_for("20230601", "20241231", "20240301")),
                                              cratchit::FiscalYear{"2024-01-01", "2024-12-31"});
  ASSERT_FALSE(plan);
  EXPECT_EQ(plan.error(), "SIE-filen gäller räkenskapsåret 2023-06-01 – 2024-12-31, inte 2024");
}

TEST(FiscalYearBefore, IsTwelveMonthsEndingTheDayBefore) {
  EXPECT_EQ(cratchit::year_before({"2025-05-01", "2026-04-30"}), (cratchit::FiscalYear{"2024-05-01", "2025-04-30"}));
  EXPECT_EQ(cratchit::year_before({"2026-11-01", "2026-12-31"}), (cratchit::FiscalYear{"2025-11-01", "2026-10-31"}));
  EXPECT_EQ(cratchit::months_of({"2023-07-01", "2024-12-31"}), 18);
  EXPECT_EQ(cratchit::months_of({"2026-01-01", "2026-04-30"}), 4);
}

TEST_F(SieImportTest, ReplacingMakesTheYearExactlyTheFilesAndKeepsItsDrafts) {
  // The file has another A 1 and lacks A 2: both in the way
  auto const file = write("less.se", sie_file(2025, "", sale(2025, 1, "0201")));
  auto const plan = cratchit::plan_sie_import(project, file);
  ASSERT_TRUE(plan) << plan.error();
  EXPECT_FALSE(plan->possible());
  ASSERT_TRUE(plan->replaceable);
  EXPECT_FALSE(plan->year_after);
  auto const replaced = cratchit::replace_year_with_sie(project, *plan, *cratchit::parse_iso_date("2026-11-09"));
  ASSERT_TRUE(replaced) << replaced.error();
  EXPECT_EQ(replaced->removed.size(), 2u);
  EXPECT_EQ(replaced->vouchers, 1u);
  EXPECT_EQ(replaced->entry.use, "ersätt");
  auto const books = cratchit::books::read_books(project.folder);
  ASSERT_TRUE(books) << books.error();
  ASSERT_EQ(books->years[0].vouchers.size(), 1u);
  EXPECT_EQ(books->years[0].vouchers[0].event.amount, 10000); // the file's A 1: 100.00
  EXPECT_TRUE(fs::exists(file));
  // Now the file is the books: importing it again has nothing new
  auto const again = cratchit::plan_sie_import(project, file);
  ASSERT_TRUE(again) << again.error();
  EXPECT_EQ(again->blockers, (std::vector<std::string>{"Allt i SIE-filen finns redan i bokföringen."}));
}

TEST_F(SieImportTest, AnotherTextOfAVoucherIsReplaceable) {
  auto file = with_a2_rows(struck_and_added);
  file.replace(file.find("\"Bank cost\""), 11, "\"Bank fee\"");
  auto const plan = cratchit::plan_sie_import(project, write("text.se", file));
  ASSERT_TRUE(plan) << plan.error();
  ASSERT_TRUE(plan->replaceable);
  ASSERT_EQ(plan->in_the_way.size(), 1u);
  EXPECT_EQ(plan->in_the_way[0].number, "2");
  EXPECT_EQ(plan->in_the_way[0].why, "har annat datum eller annan text i filen");
}

TEST_F(SieImportTest, AnotherOrgnrIsNotReplaceable) {
  auto const plan = cratchit::plan_sie_import(
      project, write("other.se", sie_file(2025, "", sale(2025, 1, "0201"), "\"Test AB\"", "556000-0002")));
  ASSERT_TRUE(plan) << plan.error();
  EXPECT_FALSE(plan->replaceable);
}

TEST(YearsBeforeCurrent, CountsTheYearsAfterUntilTheOneTodayLiesIn) {
  cratchit::FiscalYear const year{"2025-05-01", "2026-04-30"};
  EXPECT_EQ(cratchit::years_before_current(year, "2025-06-01"), 0);
  EXPECT_EQ(cratchit::years_before_current(year, "2026-04-30"), 0);
  EXPECT_EQ(cratchit::years_before_current(year, "2026-05-01"), 1);
  EXPECT_EQ(cratchit::years_before_current(year, "2026-11-15"), 1);
  EXPECT_EQ(cratchit::years_before_current({"2017-05-01", "2018-04-30"}, "2026-11-15"), 9);
  EXPECT_EQ(cratchit::years_before_current({"2016-05-01", "2017-04-30"}, "2026-11-15"), 10);
  // A first year of another length counts as one
  EXPECT_EQ(cratchit::years_before_current({"2025-01-01", "2025-04-30"}, "2026-11-15"), 2);
}
