#include "core/inventory.hpp"

#include "core/sha256.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

// A project folder of its own for each test, removed afterwards
class InventoryTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_inventory_") + info->name());
    fs::remove_all(root);
    project = root / "Test AB";
    fs::create_directories(project / ".cratchit");
  }
  void TearDown() override { fs::remove_all(root); }

  fs::path root;
  fs::path project;
};

cratchit::InventoryEntry entry(std::string sha256, std::string name) {
  cratchit::InventoryEntry e;
  e.sha256 = std::move(sha256);
  e.kind = "sie";
  e.name = std::move(name);
  return e;
}

cratchit::Date day(char const* text) { return *cratchit::parse_iso_date(text); }

} // namespace

TEST_F(InventoryTest, IsEmptyWhenThereIsNoFile) {
  auto const inventory = cratchit::read_inventory(project, day("2025-01-01"));
  ASSERT_TRUE(inventory) << inventory.error();
  EXPECT_TRUE(inventory->empty());
  EXPECT_FALSE(fs::exists(cratchit::inventory_file(project))); // reading writes nothing
}

TEST_F(InventoryTest, EntriesAreAddedLastAndReadBackWithAHeaderRow) {
  auto const file = cratchit::year_inventory_file(project, day("2025-01-01"));
  EXPECT_EQ(file, project / ".cratchit" / "books" / "2025-01-01" / "inventory.csv");
  ASSERT_TRUE(cratchit::add_to_inventory(file, entry("aa", "a.se")));
  ASSERT_TRUE(cratchit::add_to_inventory(file, entry("bb", "b, med komma.se")));
  EXPECT_EQ(cratchit::read_inventory_file(file),
            (std::vector<cratchit::InventoryEntry>{entry("aa", "a.se"), entry("bb", "b, med komma.se")}));
  std::ifstream in(file);
  std::string first;
  std::getline(in, first);
  EXPECT_EQ(first, "sha256,kind,name,path,size,read,use,generated,program,year_start,year_end,status,vouchers_added,"
                   "vouchers_changed");
}

TEST_F(InventoryTest, AFiscalYearsEntriesOverrideTheProjectsForTheSameFile) {
  ASSERT_TRUE(cratchit::add_to_inventory(cratchit::inventory_file(project), entry("aa", "allmän a.se")));
  ASSERT_TRUE(cratchit::add_to_inventory(cratchit::inventory_file(project), entry("bb", "allmän b.se")));
  ASSERT_TRUE(cratchit::add_to_inventory(cratchit::year_inventory_file(project, day("2025-01-01")), entry("bb", "årets b.se")));
  EXPECT_EQ(cratchit::read_inventory(project),
            (std::vector<cratchit::InventoryEntry>{entry("aa", "allmän a.se"), entry("bb", "allmän b.se")}));
  EXPECT_EQ(cratchit::read_inventory(project, day("2025-01-01")),
            (std::vector<cratchit::InventoryEntry>{entry("aa", "allmän a.se"), entry("bb", "årets b.se")}));
  EXPECT_EQ(cratchit::read_inventory(project, day("2024-01-01")),
            (std::vector<cratchit::InventoryEntry>{entry("aa", "allmän a.se"), entry("bb", "allmän b.se")}));
}

TEST_F(InventoryTest, ARowWithAnotherNumberOfFieldsIsAnError) {
  std::ofstream(cratchit::inventory_file(project)) << "aa,sie\n";
  EXPECT_FALSE(cratchit::read_inventory(project));
}

TEST_F(InventoryTest, AnSieUseIsRecordedInTheInventoryOfItsYearLeavingTheFileAlone) {
  fs::create_directories(project / "SIE-filer");
  // A u8 literal: a narrow one is read in the ANSI code page on Windows, which would make another name on disk
  auto const sie = project / "SIE-filer" / fs::path(u8"årets.se");
  std::ofstream(sie, std::ios::binary)
      << "#FLAGGA 0\r\n#PROGRAM \"Fortnox\" 3.62\r\n#SIETYP 4\r\n#GEN 20260115\r\n#FNAMN \"Test AB\"\r\n"
         "#RAR 0 20250101 20251231\r\n";
  auto const before = fs::last_write_time(sie);
  auto const recorded = cratchit::record_sie_use(project, sie, {"import", 3, 1}, day("2026-11-09"));
  ASSERT_TRUE(recorded) << recorded.error();
  EXPECT_EQ(recorded->sha256, cratchit::sha256_of_file(sie).value());
  EXPECT_EQ(recorded->kind, "sie");
  EXPECT_EQ(recorded->name, "årets.se");
  EXPECT_EQ(recorded->path, "SIE-filer/årets.se");
  EXPECT_EQ(recorded->size, std::to_string(fs::file_size(sie)));
  EXPECT_EQ(recorded->read, "2026-11-09");
  EXPECT_EQ(recorded->use, "import");
  EXPECT_EQ(recorded->generated, "2026-01-15");
  EXPECT_EQ(recorded->program, "Fortnox 3.62");
  EXPECT_EQ(recorded->year_start, "2025-01-01");
  EXPECT_EQ(recorded->year_end, "2025-12-31");
  EXPECT_EQ(recorded->status, "fullbordat"); // no balances, so it balances
  EXPECT_EQ(recorded->vouchers_added, "3");
  EXPECT_EQ(recorded->vouchers_changed, "1");
  EXPECT_EQ(cratchit::read_inventory(project, day("2025-01-01")), (std::vector<cratchit::InventoryEntry>{*recorded}));
  EXPECT_EQ(fs::last_write_time(sie), before);
}
