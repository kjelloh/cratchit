#include "core/fiscal_year.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

using cratchit::FiscalYear;
using cratchit::label_of;

namespace {

// A project folder of its own for each test, removed afterwards
class FiscalYearTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    folder = fs::temp_directory_path() / (std::string("cratchit_fiscal_year_") + info->name());
    fs::remove_all(folder);
    fs::create_directories(folder / ".cratchit");
  }
  void TearDown() override { fs::remove_all(folder); }

  cratchit::Project project() const { return {folder, folder / ".cratchit" / "project.txt", "1", "AB"}; }

  void books(std::string const& text) const {
    fs::create_directories(folder / ".cratchit" / "books");
    std::ofstream(folder / ".cratchit" / "books" / "books.txt") << text;
  }

  fs::path folder;
};

} // namespace

TEST_F(FiscalYearTest, NoBooksMeansNoFiscalYears) {
  auto const years = cratchit::list_fiscal_years(project());
  ASSERT_TRUE(years) << years.error();
  EXPECT_TRUE(years->empty());
  EXPECT_FALSE(fs::exists(folder / ".cratchit" / "books"));
}

TEST_F(FiscalYearTest, YearsAreReadFromBooksTheLatestFirst) {
  books("chart_type = EUBAS97\nyear.2024-05-01 = 2025-04-30\nyear.2026-05-01 = 2027-04-30\nyear.2025-05-01 = 2026-04-30\n");
  auto const years = cratchit::list_fiscal_years(project());
  ASSERT_TRUE(years) << years.error();
  EXPECT_EQ(*years, (std::vector<FiscalYear>{{"2026-05-01", "2027-04-30"},
                                             {"2025-05-01", "2026-04-30"},
                                             {"2024-05-01", "2025-04-30"}}));
}

TEST_F(FiscalYearTest, DateThatIsNotIsoIsAnError) {
  books("year.2025-05-01 = 30 april 2026\n");
  auto const years = cratchit::list_fiscal_years(project());
  ASSERT_FALSE(years);
  EXPECT_NE(years.error().find("books.txt"), std::string::npos);
}

TEST(FiscalYearLabel, CalendarYearIsOneYear) { EXPECT_EQ(label_of({"2025-01-01", "2025-12-31"}), "2025"); }

TEST(FiscalYearLabel, BrokenYearIsTwoYears) { EXPECT_EQ(label_of({"2025-05-01", "2026-04-30"}), "2025/26"); }

TEST(FiscalYearLabel, AcrossACenturyBothAreWhole) { EXPECT_EQ(label_of({"1999-07-01", "2000-06-30"}), "1999/2000"); }

// A year is listed from books.txt alone: whether it has any books (a folder of
// its own with vouchers) does not matter
TEST_F(FiscalYearTest, YearWithoutBooksIsListed) {
  books("year.2024-05-01 = 2025-04-30\nyear.2025-05-01 = 2026-04-30\n");
  fs::create_directories(folder / ".cratchit" / "books" / "2024-05-01");
  std::ofstream(folder / ".cratchit" / "books" / "2024-05-01" / "vouchers.csv") << "series,number,date,text,amount\n";
  // 2025-05-01 has no folder at all
  auto const years = cratchit::list_fiscal_years(project());
  ASSERT_TRUE(years) << years.error();
  EXPECT_EQ(*years, (std::vector<FiscalYear>{{"2025-05-01", "2026-04-30"}, {"2024-05-01", "2025-04-30"}}));
}

TEST(YearAfter, IsTwelveMonthsFromTheDayAfter) {
  EXPECT_EQ(cratchit::year_after({"2025-05-01", "2026-04-30"}), (FiscalYear{"2026-05-01", "2027-04-30"}));
  EXPECT_EQ(cratchit::year_after({"2026-11-01", "2026-12-31"}), (FiscalYear{"2027-01-01", "2027-12-31"})); // after a short first year
  EXPECT_EQ(cratchit::year_after({"2026-07-01", "2027-12-31"}), (FiscalYear{"2028-01-01", "2028-12-31"})); // after a long one
  EXPECT_EQ(cratchit::year_after({"2027-03-01", "2028-02-29"}), (FiscalYear{"2028-03-01", "2029-02-28"}));
}

namespace {

std::vector<FiscalYear> years_of(std::vector<cratchit::ListedYear> const& listed) {
  std::vector<FiscalYear> result;
  for (auto const& year : listed) result.push_back(year.year);
  return result;
}

} // namespace

TEST_F(FiscalYearTest, YearsUpToTodayAreListedTheCurrentFirst) {
  books("year.2024-05-01 = 2025-04-30\nyear.2025-05-01 = 2026-04-30\n");
  auto const listed = cratchit::list_years_until(project(), "2026-11-09");
  ASSERT_TRUE(listed) << listed.error();
  EXPECT_EQ(years_of(*listed), (std::vector<FiscalYear>{{"2026-05-01", "2027-04-30"},
                                                        {"2025-05-01", "2026-04-30"},
                                                        {"2024-05-01", "2025-04-30"},
                                                        {"2023-05-01", "2024-04-30"}})); // the year before, to import into
  EXPECT_FALSE((*listed)[0].in_books);
  EXPECT_TRUE((*listed)[1].in_books);
  EXPECT_FALSE((*listed)[3].in_books);
  EXPECT_FALSE(fs::exists(folder / ".cratchit" / "books" / "2026-05-01")); // nothing written
}

TEST_F(FiscalYearTest, ANewYearShowsWhenTheDayIsInIt) {
  books("year.2025-05-01 = 2026-04-30\n");
  EXPECT_EQ(cratchit::list_years_until(project(), "2026-04-30")->size(), 2u); // and the year before
  EXPECT_EQ(cratchit::list_years_until(project(), "2026-05-01")->size(), 3u);
  EXPECT_EQ(cratchit::list_years_until(project(), "2028-05-01")->size(), 5u); // years without books in between
}

TEST_F(FiscalYearTest, ALaterYearInTheBooksIsTheCurrentOne) {
  books("year.2027-01-01 = 2027-12-31\n"); // a company starting next year
  auto const listed = cratchit::list_years_until(project(), "2026-11-09");
  ASSERT_TRUE(listed) << listed.error();
  EXPECT_EQ(years_of(*listed), (std::vector<FiscalYear>{{"2027-01-01", "2027-12-31"}, {"2026-01-01", "2026-12-31"}}));
}

TEST_F(FiscalYearTest, YearsFollowAShortFirstYear) {
  books("year.2026-11-01 = 2026-12-31\n");
  auto const listed = cratchit::list_years_until(project(), "2028-02-01");
  ASSERT_TRUE(listed) << listed.error();
  EXPECT_EQ(years_of(*listed), (std::vector<FiscalYear>{{"2028-01-01", "2028-12-31"},
                                                        {"2027-01-01", "2027-12-31"},
                                                        {"2026-11-01", "2026-12-31"},
                                                        {"2025-11-01", "2026-10-31"}}));
}

TEST_F(FiscalYearTest, NoYearsInTheBooksListsNone) {
  auto const listed = cratchit::list_years_until(project(), "2026-11-09");
  ASSERT_TRUE(listed) << listed.error();
  EXPECT_TRUE(listed->empty());
}
