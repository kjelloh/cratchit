#include "cli.hpp"

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

// What run_cli wrote to out and err, and its exit code
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
  result.code = walk_pdf::run_cli(arguments, out, err, "9.9.9");
  result.out = read_back(out);
  result.err = read_back(err);
  return result;
}

// A one-page PDF with a text, as walk_pdf's own tests make them
std::string pdf(std::string_view text) {
  std::string widths;
  for (int i = 0; i < 224; ++i) widths += "500 ";
  std::string const content = "BT /F1 10 Tf 72 700 Td (" + std::string(text) + ") Tj ET";
  return std::string{"%PDF-1.4\n"
                     "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n"
                     "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n"
                     "3 0 obj\n<< /Type /Page /Parent 2 0 R /Contents 4 0 R"
                     " /Resources << /Font << /F1 5 0 R >> >> >>\nendobj\n"
                     "4 0 obj\n<< /Length "} +
         std::to_string(content.size()) + " >>\nstream\n" + content +
         "\nendstream\nendobj\n"
         "5 0 obj\n<< /Type /Font /Subtype /Type1 /Encoding /WinAnsiEncoding /FirstChar 32 /Widths [" +
         widths + "] >>\nendobj\ntrailer\n<< /Size 6 /Root 1 0 R >>\n%%EOF\n";
}

class CliTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Its own file for each test, as ctest runs them at the same time
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    file = fs::temp_directory_path() / (std::string("cratchit_walk_pdf_cli_test_") + info->name() + ".pdf");
    std::ofstream(file, std::ios::binary) << pdf("Faktura 123");
  }
  void TearDown() override { fs::remove(file); }
  fs::path file;
};

} // namespace

TEST(Cli, NoArgumentsGiveTheVersions) {
  auto const result = run({});
  EXPECT_EQ(result.code, 0);
  EXPECT_EQ(result.out, "walk_pdf 0.1.0 (cratchit 9.9.9)\n");
}

TEST(Cli, HelpGivesTheManual) {
  for (std::string_view const help : {"-h", "--help"}) {
    auto const result = run({help});
    EXPECT_EQ(result.code, 0);
    EXPECT_TRUE(result.out.starts_with("# walk_pdf\n")) << result.out.substr(0, 40);
    EXPECT_NE(result.out.find("cratchit --walk-pdf --text <file.pdf>"), std::string::npos);
  }
}

TEST(Cli, InvalidArgumentsAreExitCodeTwo) {
  auto result = run({"--depth"});
  EXPECT_EQ(result.code, 2);
  EXPECT_EQ(result.err, "Error: --depth needs a number\nSee cratchit --walk-pdf --help\n");
  EXPECT_EQ(run({"a.pdf", "/", "x"}).code, 2);
  EXPECT_EQ(run({"--bogus"}).code, 2);
}

TEST_F(CliTest, TextGoesToOut) {
  auto const result = run({"--text", file.string()});
  EXPECT_EQ(result.code, 0);
  EXPECT_NE(result.out.find("Faktura 123"), std::string::npos) << result.out;
  EXPECT_TRUE(result.err.empty()) << result.err;
}

TEST_F(CliTest, WalksTheStructure) {
  auto const result = run({file.string(), "--depth", "1"});
  EXPECT_EQ(result.code, 0);
  EXPECT_NE(result.out.find("/rev[0]"), std::string::npos) << result.out;
  EXPECT_EQ(run({file.string(), "/rev[9]"}).code, 1);
}

TEST(Cli, AMissingFileIsExitCodeOne) {
  auto const result = run({"--text", "/no/such/file.pdf"});
  EXPECT_EQ(result.code, 1);
  EXPECT_TRUE(result.err.starts_with("Error: '/no/such/file.pdf': ")) << result.err;
}
