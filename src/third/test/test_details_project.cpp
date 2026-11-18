#include "core/details_project.hpp"

#include "core/books/books.hpp"
#include "core/path_value.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using cratchit::FiscalYear;
using cratchit::parse_orgnr;

TEST(Orgnr, TakesTenDigitsWithOrWithoutTheDash) {
  EXPECT_EQ(parse_orgnr("556677-8899"), "556677-8899");
  EXPECT_EQ(parse_orgnr("5566778899"), "556677-8899");
  EXPECT_EQ(parse_orgnr(" 556782-8172 "), "556782-8172");
}

TEST(Orgnr, RefusesAWrongCheckDigit) {
  auto const orgnr = parse_orgnr("556677-8898");
  ASSERT_FALSE(orgnr);
  EXPECT_TRUE(orgnr.error().starts_with("Kontrollsiffran")) << orgnr.error();
}

TEST(Orgnr, RefusesWhatIsNoOrgnr) {
  EXPECT_FALSE(parse_orgnr(""));
  EXPECT_FALSE(parse_orgnr("556677-889"));
  EXPECT_FALSE(parse_orgnr("55667788990"));
  EXPECT_FALSE(parse_orgnr("5566-778899"));
  EXPECT_FALSE(parse_orgnr("556677 8899"));
  EXPECT_FALSE(parse_orgnr("abcdef-ghij"));
}

TEST(FirstMonth, IsYearAndMonth) {
  EXPECT_EQ(cratchit::parse_first_month("2026-05"), std::chrono::year{2026} / std::chrono::May);
  EXPECT_EQ(cratchit::parse_first_month(" 2026-12 "), std::chrono::year{2026} / std::chrono::December);
  EXPECT_FALSE(cratchit::parse_first_month(""));
  EXPECT_FALSE(cratchit::parse_first_month("2026"));
  EXPECT_FALSE(cratchit::parse_first_month("2026-13"));
  EXPECT_FALSE(cratchit::parse_first_month("2026-05-01"));
  EXPECT_FALSE(cratchit::parse_first_month("maj 2026"));
}

TEST(FirstYearLength, IsOneToEighteenMonths) {
  EXPECT_EQ(cratchit::parse_first_year_length("12"), 12);
  EXPECT_EQ(cratchit::parse_first_year_length(" 1 "), 1);
  EXPECT_EQ(cratchit::parse_first_year_length("18"), 18);
  EXPECT_FALSE(cratchit::parse_first_year_length("0"));
  EXPECT_FALSE(cratchit::parse_first_year_length("19"));
  EXPECT_FALSE(cratchit::parse_first_year_length(""));
  EXPECT_FALSE(cratchit::parse_first_year_length("tolv"));
  EXPECT_FALSE(cratchit::parse_first_year_length("12 månader"));
}

TEST(FirstFiscalYear, StartsFirstInTheMonthAndEndsLastInTheLastMonth) {
  using std::chrono::year;
  EXPECT_EQ(cratchit::first_fiscal_year(year{2026} / std::chrono::January, 12), (FiscalYear{"2026-01-01", "2026-12-31"}));
  EXPECT_EQ(cratchit::first_fiscal_year(year{2026} / std::chrono::May, 12), (FiscalYear{"2026-05-01", "2027-04-30"}));
  EXPECT_EQ(cratchit::first_fiscal_year(year{2026} / std::chrono::July, 18), (FiscalYear{"2026-07-01", "2027-12-31"}));
  EXPECT_EQ(cratchit::first_fiscal_year(year{2026} / std::chrono::November, 2), (FiscalYear{"2026-11-01", "2026-12-31"}));
  EXPECT_EQ(cratchit::first_fiscal_year(year{2027} / std::chrono::March, 12), (FiscalYear{"2027-03-01", "2028-02-29"}));
}

namespace {

// A workspace folder of its own for each test, removed afterwards
class DetailsProjectTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_details_project_") + info->name());
    fs::remove_all(root);
    fs::create_directories(root);
  }
  void TearDown() override { fs::remove_all(root); }

  cratchit::Workspace workspace() const { return {root, root / ".cratchit" / "workspace.txt", {}}; }

  fs::path root;
};

} // namespace

TEST_F(DetailsProjectTest, FolderIsNamedAfterTheCompanyInTheWorkspace) {
  EXPECT_EQ(cratchit::new_project_folder(workspace(), "Exempel AB"), root / "Exempel AB");
  EXPECT_FALSE(cratchit::new_project_folder(workspace(), ""));
  EXPECT_FALSE(cratchit::new_project_folder(workspace(), "A/B"));
  fs::create_directories(root / "Finns AB");
  EXPECT_FALSE(cratchit::new_project_folder(workspace(), "Finns AB"));
}

TEST_F(DetailsProjectTest, CreatesProjectAndBooksWithTheYear) {
  auto const folder = root / "Exempel AB";
  auto const project =
      cratchit::create_project_from_details(folder, "Exempel AB", "556677-8899", {"2026-05-01", "2027-04-30"}, "2026-11-09");
  ASSERT_TRUE(project) << project.error();
  auto const entries = cratchit::read_path_values(folder / ".cratchit" / "project.txt");
  ASSERT_TRUE(entries) << entries.error();
  EXPECT_EQ(cratchit::find_value(*entries, "created"), "2026-11-09");
  EXPECT_EQ(cratchit::find_value(*entries, "uuid"), project->uuid);
  EXPECT_EQ(cratchit::find_value(*entries, "name"), "Exempel AB");
  EXPECT_EQ(cratchit::find_value(*entries, "orgnr"), "556677-8899");
  EXPECT_TRUE(cratchit::open_project(folder, project->uuid));
  EXPECT_EQ(cratchit::list_fiscal_years(*project), (std::vector<FiscalYear>{{"2026-05-01", "2027-04-30"}}));
  auto const books = cratchit::books::read_books(folder);
  ASSERT_TRUE(books) << books.error();
  EXPECT_EQ(books->years.size(), 1u);
  EXPECT_TRUE(books->accounts.empty());
  // Only project.txt and books.txt
  std::vector<fs::path> files;
  for (auto const& entry : fs::recursive_directory_iterator(folder)) {
    if (entry.is_regular_file()) files.push_back(fs::relative(entry.path(), folder));
  }
  std::ranges::sort(files);
  EXPECT_EQ(files, (std::vector<fs::path>{fs::path(".cratchit") / "books" / "books.txt", fs::path(".cratchit") / "project.txt"}));
  auto const books_txt = cratchit::read_path_values(folder / ".cratchit" / "books" / "books.txt");
  ASSERT_TRUE(books_txt) << books_txt.error();
  EXPECT_EQ(books_txt->size(), 1u);
}

// A project in the working folder itself

namespace {

// An empty folder of its own for each test, removed afterwards
struct HereFolder {
  HereFolder() {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_here_test_") + info->name());
    fs::remove_all(root);
    fs::create_directories(root);
  }
  ~HereFolder() { fs::remove_all(root); }
  fs::path root;
};

} // namespace

TEST(ProjectHere, IsAllowedInAFolderWithoutDotFolder) {
  HereFolder here;
  fs::create_directories(here.root / "company" / "Avtal");
  EXPECT_TRUE(cratchit::check_project_here(here.root / "company", here.root / "workspace", here.root / "home"));
}

TEST(ProjectHere, IsRefusedInHomeWorkspaceADotFolderAndInsideAProject) {
  HereFolder here;
  fs::create_directories(here.root / "home");
  fs::create_directories(here.root / "workspace");
  fs::create_directories(here.root / "dotted" / ".cratchit");
  fs::create_directories(here.root / "project" / ".cratchit");
  std::ofstream(here.root / "project" / ".cratchit" / "project.txt") << "uuid = 1\nname = P\n";
  fs::create_directories(here.root / "project" / "inside" / "deeper");
  auto const refused = [&](fs::path const& folder, std::string const& start) {
    auto const checked = cratchit::check_project_here(folder, here.root / "workspace", here.root / "home");
    ASSERT_FALSE(checked) << folder.string();
    EXPECT_TRUE(checked.error().starts_with(start)) << checked.error();
  };
  refused(here.root / "home", "Arbetsmappen är din hemkatalog");
  refused(here.root / "workspace", "Arbetsmappen är samlingsmappen");
  refused(here.root / "dotted", "Arbetsmappen har redan en .cratchit-mapp");
  refused(here.root / "project", "Arbetsmappen har redan en .cratchit-mapp");
  refused(here.root / "project" / "inside" / "deeper", "Arbetsmappen ligger inuti bokföringen i");
}

TEST(ProjectHere, IsCreatedInTheFolderItselfLeavingTheRestAlone) {
  HereFolder here;
  auto const folder = here.root / "The ITfied";
  fs::create_directories(folder / "Avtal");
  std::ofstream(folder / "brev.txt") << "hej\n";
  auto const project = cratchit::create_project_here(folder, "The ITfied AB", "556782-8172",
                                                     FiscalYear{"2026-05-01", "2027-04-30"}, "2026-11-15");
  ASSERT_TRUE(project) << project.error();
  EXPECT_EQ(project->folder, folder);
  EXPECT_EQ(project->name, "The ITfied AB");
  auto const entries = cratchit::read_path_values(folder / ".cratchit" / "project.txt");
  ASSERT_TRUE(entries);
  EXPECT_EQ(cratchit::find_value(*entries, "name"), "The ITfied AB");
  EXPECT_EQ(cratchit::find_value(*entries, "orgnr"), "556782-8172");
  EXPECT_TRUE(fs::is_regular_file(folder / ".cratchit" / "books" / "books.txt"));
  EXPECT_TRUE(fs::is_directory(folder / "Avtal"));
  EXPECT_TRUE(fs::is_regular_file(folder / "brev.txt"));
  // Not twice
  EXPECT_FALSE(cratchit::create_project_here(folder, "The ITfied AB", "556782-8172",
                                             FiscalYear{"2026-05-01", "2027-04-30"}, "2026-11-15"));
}
