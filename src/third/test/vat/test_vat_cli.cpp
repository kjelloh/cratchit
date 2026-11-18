#include "core/skv/vat_return/cli.hpp"

#include "temporary_file.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct Run {
  int code = 0;
  std::string out;
  std::string err;
};

std::string read_back(std::FILE* file) {
  std::rewind(file);
  std::string text;
  for (int c; (c = std::fgetc(file)) != EOF;) text += static_cast<char>(c);
  std::fclose(file);
  return text;
}

Run run(std::vector<std::string_view> const& arguments) {
  auto* out = test::temporary_file();
  auto* err = test::temporary_file();
  Run result;
  result.code = cratchit::skv::vat_return::run_cli(arguments, out, err, "9.9.9");
  result.out = read_back(out);
  result.err = read_back(err);
  return result;
}

std::string text_of(fs::path const& file) {
  std::ifstream in(file, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

// Books of 2025: a sale with 25 % VAT in Q1, cleared by M1 on its last day
class VatCliTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    folder = fs::temp_directory_path() / (std::string("cratchit_vat_cli_test_") + info->name());
    fs::remove_all(folder);
    fs::create_directories(folder);
    sie = folder / "books.se";
    std::ofstream(sie, std::ios::binary)
        << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
           "#KONTO 1930 \"Bank\"\r\n#KONTO 2611 \"Utgaende moms 25\"\r\n#KONTO 2650 \"Momsredovisning\"\r\n"
           "#KONTO 3001 \"Forsaljning\"\r\n"
           "#VER B 1 20250210 \"Sale\"\r\n{\r\n#TRANS 1930 {} 1250\r\n#TRANS 3001 {} -1000\r\n#TRANS 2611 {} -250\r\n}\r\n"
           "#VER M 1 20250331 \"Momsrapport 2025-01-01 - 2025-03-31\"\r\n{\r\n#TRANS 2611 {} 250\r\n#TRANS 2650 {} -250\r\n}\r\n";
    previous = fs::current_path();
    fs::current_path(folder);
  }
  void TearDown() override {
    fs::current_path(previous);
    fs::remove_all(folder);
  }
  fs::path folder;
  fs::path sie;
  fs::path previous;
};

} // namespace

TEST(VatCli, InvalidArgumentsAreExitCodeTwo) {
  auto const result = run({});
  EXPECT_EQ(result.code, 2);
  EXPECT_TRUE(result.err.starts_with("Error: No SIE file given\nUsage: cratchit --sie-to-vat")) << result.err;
  EXPECT_EQ(run({"a.se", "--period", "2025Q5"}).code, 2);
  EXPECT_EQ(run({"a.se", "b.se", "c.se"}).code, 2);
  EXPECT_EQ(run({"--bogus"}).code, 2);
}

TEST(VatCli, HelpGivesTheManual) {
  auto const result = run({"--help"});
  EXPECT_EQ(result.code, 0);
  EXPECT_TRUE(result.out.starts_with("# cratchit --sie-to-vat")) << result.out.substr(0, 40);
}

TEST_F(VatCliTest, ListsTheQuarters) {
  auto const result = run({sie.string()});
  EXPECT_EQ(result.code, 0) << result.err;
  EXPECT_NE(result.out.find("  2025 Q1      -250.00     -250.00        0.00  cleared by M1"), std::string::npos)
      << result.out;
}

TEST_F(VatCliTest, WritesTheQuarterAsFiledEvenWhenCleared) {
  auto const result = run({sie.string(), "--period", "2025Q1"});
  ASSERT_EQ(result.code, 0) << result.err;
  auto const stem = std::string("556000-0001_moms_2025-01-01_2025-03-31");
  auto const eskd = text_of(folder / stem / "moms_202503.eskd");
  EXPECT_NE(eskd.find("<MomsUtgHog>250</MomsUtgHog>"), std::string::npos) << eskd;
  EXPECT_NE(eskd.find("<MomsBetala>250</MomsBetala>"), std::string::npos) << eskd;
  auto const report = text_of(folder / stem / (stem + "_momsrapport.txt"));
  EXPECT_TRUE(report.starts_with("Momsrapport 2025-01-01 – 2025-03-31\nTest AB, 556000-0001\n")) << report;
  EXPECT_NE(report.find("Ruta 49 på blanketten: 250.00 att betala."), std::string::npos);
  EXPECT_NE(text_of(folder / stem / (stem + "_verifikation.txt")).find("redan avräknad av M1"), std::string::npos);
  EXPECT_TRUE(fs::exists(folder / stem / (stem + ".se")));
}

TEST_F(VatCliTest, AQuarterBeforeTheFilesNeedsLastYearsFile) {
  auto const result = run({sie.string(), "--period", "2024Q4"});
  EXPECT_EQ(result.code, 1);
  EXPECT_FALSE(result.err.empty());
}
