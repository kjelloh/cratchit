#include "core/sie_summary.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace {

// An SIE file of its own for each test, removed afterwards
struct SieFile {
  explicit SieFile(std::string const& text) {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    path = fs::temp_directory_path() / (std::string("cratchit_sie_summary_") + info->name() + ".se");
    std::ofstream(path, std::ios::binary) << text;
  }
  ~SieFile() { fs::remove(path); }
  fs::path path;
};

std::string voucher(std::string const& series, std::string const& number, std::string const& date) {
  return "#VER " + series + " " + number + " " + date + " \"T\"\r\n{\r\n#TRANS 1930 {} 1.00\r\n#TRANS 3010 {} -1.00\r\n}\r\n";
}

} // namespace

TEST(SieSummary, GivesYearGeneratedVouchersLastDateAndHighestNumberPerSeries) {
  SieFile file("#FLAGGA 0\r\n#SIETYP 4\r\n#GEN 20261112\r\n#FNAMN \"Test AB\"\r\n#RAR 0 20250501 20260430\r\n"
               "#RAR -1 20240501 20250430\r\n" +
               voucher("A", "9", "20250601") + voucher("A", "10", "20260115") + voucher("M", "3", "20260430") +
               voucher("D", "1", "20250701"));
  auto const summary = cratchit::summarize_sie_file(file.path);
  ASSERT_TRUE(summary) << summary.error();
  EXPECT_EQ(summary->vouchers, 4u);
  EXPECT_EQ(cratchit::summary_line(*summary, *cratchit::parse_iso_date("2026-11-09")),
            "2025-05-01 – 2026-04-30  ofullbordat  skapad 2026-11-12  4 ver. t.o.m. 2026-04-30  A10 · D1 · M3");
  // The tests' today is 2026-11-09 (test_today.cpp)
  EXPECT_EQ(cratchit::describe_sie_file(file.path),
            (std::vector<std::string>{cratchit::summary_line(*summary, *cratchit::parse_iso_date("2026-11-09"))}));
}

TEST(SieSummary, LeavesOutWhatTheFileLacks) {
  SieFile file("#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n");
  auto const summary = cratchit::summarize_sie_file(file.path);
  ASSERT_TRUE(summary) << summary.error();
  EXPECT_EQ(cratchit::summary_line(*summary, *cratchit::parse_iso_date("2026-11-09")), "0 ver.");
}

TEST(SieSummary, SaysWhyAFileCannotBeRead) {
  SieFile file("Det här är ingen SIE-fil\r\n");
  EXPECT_EQ(cratchit::describe_sie_file(file.path), (std::vector<std::string>{"(går inte att läsa: #SIETYP saknas)"}));
  EXPECT_EQ(cratchit::describe_sie_file(file.path.parent_path() / "finns_inte.se"),
            (std::vector<std::string>{"(går inte att läsa: filen går inte att öppna)"}));
}

// Listing the files found with Tab

namespace {

// Files of their own for each test, in a folder removed afterwards
struct SieFolder {
  SieFolder() {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_sie_list_") + info->name());
    fs::remove_all(root);
    fs::create_directories(root);
  }
  ~SieFolder() { fs::remove_all(root); }
  fs::path add(std::string const& name, std::string const& text) {
    std::ofstream(root / name, std::ios::binary) << text;
    return root / name;
  }
  fs::path root;
};

std::string sie(std::string const& year, std::string const& generated, std::string const& vouchers) {
  return "#FLAGGA 0\r\n#SIETYP 4\r\n" + (generated.empty() ? std::string() : "#GEN " + generated + "\r\n") +
         (year.empty() ? std::string() : "#RAR 0 " + year + "\r\n") + vouchers;
}

} // namespace

TEST(SieList, GroupsByFiscalYearLatestFirstAndTheLatestCreatedFirstInColumns) {
  SieFolder folder;
  auto const old_2025 = folder.add("a.se", sie("20250101 20251231", "20251001", voucher("A", "1", "20250301")));
  auto const new_2025 = folder.add("bb.se", sie("20250101 20251231", "20260115",
                                                voucher("A", "1", "20250301") + voucher("A", "2", "20251231")));
  auto const only_2026 = folder.add("c.se", sie("20260101 20261231", "", ""));
  auto const no_year = folder.add("d.se", sie("", "20260101", ""));
  auto const broken = folder.add("e.se", "ingen SIE-fil\r\n");
  fs::create_directories(folder.root / "arkiv");
  auto const lines = cratchit::list_sie_files({old_2025, new_2025, only_2026, no_year, broken}, {folder.root / "arkiv"});
  auto const separator = std::string(1, static_cast<char>(fs::path::preferred_separator));
  EXPECT_EQ(lines, (std::vector<std::string>{
                       "2026-01-01 – 2026-12-31",
                       "  c.se   pågående                        0 ver.",
                       "2025-01-01 – 2025-12-31",
                       "  bb.se  ofullbordat  skapad 2026-01-15  2 ver.  t.o.m. 2025-12-31  A2",
                       "  a.se   ofullbordat  skapad 2025-10-01  1 ver.  t.o.m. 2025-03-01  A1",
                       "Utan räkenskapsår",
                       "  d.se                skapad 2026-01-01  0 ver.",
                       "Går inte att läsa",
                       "  e.se   #SIETYP saknas",
                       "Mappar",
                       "  arkiv" + separator,
                   }));
}

// The status of the file's fiscal year

namespace {

cratchit::Date day(char const* text) { return *cratchit::parse_iso_date(text); }

cratchit::SieSummary year_ending(char const* start, char const* end, cratchit::Amount balance, bool result_booked) {
  cratchit::SieSummary summary;
  summary.year = std::pair{day(start), day(end)};
  summary.balance = balance;
  summary.result_booked = result_booked;
  return summary;
}

} // namespace

TEST(SieStatus, IsOngoingUntilTheYearHasEnded) {
  auto const summary = year_ending("2026-05-01", "2027-04-30", 1234, false);
  EXPECT_EQ(cratchit::status_text(*cratchit::status_of(summary, day("2026-11-15"))), "pågående");
  EXPECT_EQ(cratchit::status_text(*cratchit::status_of(summary, day("2027-04-30"))), "pågående");
  EXPECT_EQ(cratchit::status_text(*cratchit::status_of(summary, day("2027-05-01"))), "ofullbordat");
}

TEST(SieStatus, IsCompleteWhenTheBalanceSheetBalances) {
  EXPECT_EQ(cratchit::status_text(*cratchit::status_of(year_ending("2025-05-01", "2026-04-30", 0, true), day("2026-11-15"))),
            "fullbordat");
  // A result of 0 needs no booking on 8999
  EXPECT_EQ(cratchit::status_text(*cratchit::status_of(year_ending("2025-05-01", "2026-04-30", 0, false), day("2026-11-15"))),
            "fullbordat");
}

TEST(SieStatus, MismatchesWhenAResultIsBookedButTheBalanceSheetDoesNotBalance) {
  EXPECT_EQ(cratchit::status_text(*cratchit::status_of(year_ending("2025-05-01", "2026-04-30", 100, true), day("2026-11-15"))),
            "misstämmer");
}

TEST(SieStatus, WarnsForAnIncompleteYearOlderThanLastYear) {
  auto const last_year = year_ending("2025-05-01", "2026-04-30", 100, false);
  auto const year_before = year_ending("2024-05-01", "2025-04-30", 100, false);
  EXPECT_EQ(cratchit::status_text(*cratchit::status_of(last_year, day("2026-11-15"))), "ofullbordat");
  EXPECT_EQ(cratchit::status_text(*cratchit::status_of(year_before, day("2026-11-15"))), "ofullbordat !");
  // Last year until a year after its end
  EXPECT_EQ(cratchit::status_text(*cratchit::status_of(last_year, day("2027-04-30"))), "ofullbordat");
  EXPECT_EQ(cratchit::status_text(*cratchit::status_of(last_year, day("2027-05-01"))), "ofullbordat !");
}

TEST(SieStatus, IsNothingWithoutAFiscalYear) {
  EXPECT_FALSE(cratchit::status_of(cratchit::SieSummary{}, day("2026-11-15")));
}

TEST(SieSummary, SumsTheBalanceAccountsAndSeesTheResultBookedOn8999) {
  SieFile file("#FLAGGA 0\r\n#SIETYP 4\r\n#RAR 0 20250101 20251231\r\n#IB 0 1930 1000.00\r\n#IB 0 2081 -1000.00\r\n"
               "#VER A 1 20250201 \"Sale\"\r\n{\r\n#TRANS 1930 {} 500.00\r\n#TRANS 3010 {} -500.00\r\n}\r\n");
  auto const before = cratchit::summarize_sie_file(file.path);
  ASSERT_TRUE(before) << before.error();
  EXPECT_EQ(before->balance, 50000); // the result, 500.00, not booked
  EXPECT_FALSE(before->result_booked);
  std::ofstream(file.path, std::ios::app | std::ios::binary)
      << "#VER M 1 20251231 \"Årets resultat\"\r\n{\r\n#TRANS 8999 {} 500.00\r\n#TRANS 2099 {} -500.00\r\n}\r\n";
  auto const after = cratchit::summarize_sie_file(file.path);
  ASSERT_TRUE(after) << after.error();
  EXPECT_EQ(after->balance, 0);
  EXPECT_TRUE(after->result_booked);
}
