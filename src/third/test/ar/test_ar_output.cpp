#include "core/ar/output.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

using cratchit::ar::output_paths;
using cratchit::ar::write_outputs;
using namespace std::chrono;

namespace {

cratchit::sie::FiscalYear const fiscal_year{0, 2025y / May / 1, 2026y / April / 30};

std::string read(std::filesystem::path const& file) {
  std::ifstream in(file, std::ios::binary);
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void write(std::filesystem::path const& file, std::string const& text) {
  std::ofstream(file, std::ios::binary) << text;
}

// A fresh, empty folder for one test, removed afterwards
class ArOutput : public testing::Test {
protected:
  void SetUp() override {
    base_ = std::filesystem::temp_directory_path() /
            ("cratchit_ar_output_" + std::string(testing::UnitTest::GetInstance()->current_test_info()->name()));
    std::filesystem::remove_all(base_);
    std::filesystem::create_directories(base_);
    write(base_ / "export.se", "#FLAGGA 0\n");
  }
  void TearDown() override { std::filesystem::remove_all(base_); }

  std::filesystem::path base_;
};

} // namespace

TEST(ArOutputNames, OrgnrAndFiscalYear) {
  EXPECT_EQ(cratchit::ar::output_stem("556782-8172", fiscal_year), "556782-8172_2025-05-01_2026-04-30");
  EXPECT_EQ(cratchit::ar::output_stem("5567 82/8172", fiscal_year), "5567_82_8172_2025-05-01_2026-04-30");
  auto const paths = output_paths("/work", "556782-8172", fiscal_year);
  EXPECT_EQ(paths.folder, "/work/556782-8172_2025-05-01_2026-04-30");
  EXPECT_EQ(paths.ar, "/work/556782-8172_2025-05-01_2026-04-30/556782-8172_2025-05-01_2026-04-30_arsredovisning.txt");
  EXPECT_EQ(paths.report, "/work/556782-8172_2025-05-01_2026-04-30/556782-8172_2025-05-01_2026-04-30_rapport.txt");
  EXPECT_EQ(paths.sie, "/work/556782-8172_2025-05-01_2026-04-30/556782-8172_2025-05-01_2026-04-30.se");
}

TEST_F(ArOutput, WritesTheFolder) {
  auto const paths = output_paths(base_, "556782-8172", fiscal_year);
  auto const written = write_outputs(paths, std::string("ÅR\n"), "report\n", base_ / "export.se");
  ASSERT_TRUE(written) << written.error();
  EXPECT_TRUE(written->ar);
  EXPECT_TRUE(written->sie_copied);
  EXPECT_EQ(read(paths.ar), "ÅR\n");
  EXPECT_EQ(read(paths.report), "report\n");
  EXPECT_EQ(read(paths.sie), "#FLAGGA 0\n");
  EXPECT_EQ(std::distance(std::filesystem::directory_iterator(paths.folder), std::filesystem::directory_iterator()), 3);
}

TEST_F(ArOutput, ReplacesTheArsredovisningAndKeepsAnIdenticalSieFile) {
  auto const paths = output_paths(base_, "556782-8172", fiscal_year);
  ASSERT_TRUE(write_outputs(paths, std::string("first\n"), "report 1\n", base_ / "export.se"));
  write(paths.ar, "edited by hand\n");
  auto const again = write_outputs(paths, std::string("second\n"), "report 2\n", base_ / "export.se");
  ASSERT_TRUE(again);
  EXPECT_FALSE(again->sie_copied);
  EXPECT_EQ(read(paths.ar), "second\n");
  EXPECT_EQ(read(paths.report), "report 2\n");
}

TEST_F(ArOutput, ReplacesADifferentSieFile) {
  auto const paths = output_paths(base_, "556782-8172", fiscal_year);
  ASSERT_TRUE(write_outputs(paths, std::string("ÅR\n"), "report\n", base_ / "export.se"));
  write(base_ / "export after closing.se", "#FLAGGA 1\n");
  auto const again = write_outputs(paths, std::string("ÅR\n"), "report\n", base_ / "export after closing.se");
  ASSERT_TRUE(again);
  EXPECT_TRUE(again->sie_copied);
  EXPECT_EQ(read(paths.sie), "#FLAGGA 1\n");
}

TEST_F(ArOutput, TheCopyItselfAsInputIsKept) {
  auto const paths = output_paths(base_, "556782-8172", fiscal_year);
  ASSERT_TRUE(write_outputs(paths, std::string("ÅR\n"), "report\n", base_ / "export.se"));
  auto const again = write_outputs(paths, std::string("ÅR\n"), "report\n", paths.sie);
  ASSERT_TRUE(again);
  EXPECT_FALSE(again->sie_copied);
  EXPECT_EQ(read(paths.sie), "#FLAGGA 0\n");
}

TEST_F(ArOutput, NoArsredovisningOnErrorsLeavesTheEarlierOne) {
  auto const paths = output_paths(base_, "556782-8172", fiscal_year);
  ASSERT_TRUE(write_outputs(paths, std::string("good\n"), "report 1\n", base_ / "export.se"));
  // Books with errors, exported again: neither the årsredovisning nor its SIE file is replaced
  write(base_ / "broken.se", "#FLAGGA 1\n");
  auto const failed = write_outputs(paths, std::nullopt, "errors\n", base_ / "broken.se");
  ASSERT_TRUE(failed);
  EXPECT_FALSE(failed->ar);
  EXPECT_FALSE(failed->sie_copied);
  EXPECT_EQ(read(paths.ar), "good\n");
  EXPECT_EQ(read(paths.sie), "#FLAGGA 0\n");
  EXPECT_EQ(read(paths.report), "errors\n");
}

TEST_F(ArOutput, FirstRunWithErrorsCopiesTheSieFile) {
  auto const paths = output_paths(base_, "556782-8172", fiscal_year);
  auto const failed = write_outputs(paths, std::nullopt, "errors\n", base_ / "export.se");
  ASSERT_TRUE(failed);
  EXPECT_TRUE(failed->sie_copied);
  EXPECT_FALSE(std::filesystem::exists(paths.ar));
}

TEST_F(ArOutput, MissingSieFileIsAnError) {
  auto const paths = output_paths(base_, "556782-8172", fiscal_year);
  auto const written = write_outputs(paths, std::string("ÅR\n"), "report\n", base_ / "missing.se");
  ASSERT_FALSE(written);
  EXPECT_NE(written.error().find("missing.se"), std::string::npos);
}

TEST(ArOutputReport, HeaderErrorsNotesAndCheck) {
  cratchit::ar::ReportHeader const header{"x_arsredovisning.txt", "2026-11-02 18:04", "cratchit_ar 0.1.0", "books.se",
                                          "x.se", "#GEN 2026-11-02", "resources/ar/bas_to_ar_2024.csv", "complete"};
  auto const report = cratchit::ar::format_report(header, {}, {"Closing: one"}, "K2 check\n========\n");
  EXPECT_TRUE(report.starts_with("Report for x_arsredovisning.txt\n===============================\n\n"
                                 "Generated: 2026-11-02 18:04 by cratchit_ar 0.1.0\n"
                                 "SIE file:  books.se, copied as x.se\n"
                                 "           #GEN 2026-11-02\n"
                                 "Mapping:   resources/ar/bas_to_ar_2024.csv\n"
                                 "Status:    complete\n"))
      << report;
  EXPECT_NE(report.find("\nErrors\n------\nNone.\n"), std::string::npos);
  EXPECT_NE(report.find("\nNotes\n-----\nClosing: one\n"), std::string::npos);
  EXPECT_TRUE(report.ends_with("\nK2 check\n========\n"));
}
