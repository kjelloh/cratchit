#include "charset/detect.hpp"
#include "core/log.hpp"
#include "core/sie/file.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

using cratchit::Charset;
using cratchit::sie::is_expected_text;
using cratchit::sie::lex_bytes;
using cratchit::sie::lex_file;

namespace {

std::vector<std::uint8_t> as_bytes(std::string_view text) {
  return {text.begin(), text.end()};
}

// The text of the first field of a lexed line
std::string first_field(cratchit::sie::Lexed const& line) {
  EXPECT_TRUE(line.has_value()) << line.error();
  if (!line.has_value() || line->fields.empty()) {
    return {};
  }
  return std::get<std::string>(line->fields[0]);
}

// A folder and log file of its own for each test, closed and removed afterwards
class SieFileTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_test_") + info->name());
    fs::remove_all(root);
    fs::create_directories(root);
    ASSERT_TRUE(cratchit::log::open(root / "log.txt").has_value());
  }
  void TearDown() override {
    cratchit::log::close();
    fs::remove_all(root);
  }

  fs::path write(std::string_view name, std::string_view bytes) const {
    auto const file = root / name;
    std::ofstream out(file, std::ios::binary);
    out << bytes;
    return file;
  }

  std::string read_log() const {
    std::ifstream in(root / "log.txt");
    std::ostringstream text;
    text << in.rdbuf();
    return text.str();
  }

  fs::path root;
};

} // namespace

TEST(SieText, ExpectedCharacters) {
  for (char32_t const c : U"\t\r\n Az09#{}\"\\~\u00e5\u00e4\u00f6\u00c5\u00c4\u00d6\u00e9\u00fc\u00df\u00b0\u00a0\u00ff") {
    if (c != 0) {
      EXPECT_TRUE(is_expected_text(c)) << static_cast<std::uint32_t>(c);
    }
  }
}

TEST(SieText, UnexpectedCharacters) {
  // Controls, DEL, C1, CP437 box drawing, blocks, Greek, math, peseta and florin
  for (char32_t const c : {0x00u, 0x1Bu, 0x7Fu, 0x85u, 0x251Cu, 0x2502u, 0x2591u, 0x25A0u, 0x03A3u, 0x03C3u, 0x2248u,
                           0x20A7u, 0x0192u}) {
    EXPECT_FALSE(is_expected_text(c)) << static_cast<std::uint32_t>(c);
  }
}

TEST(SieLexBytes, Cp437IsReadAsCp437) {
  // Ö = 0x99, ö = 0x94 in CP437
  auto const result = lex_bytes(as_bytes("#FNAMN \"\x99rebro f\x94rening\"\r\n"));
  ASSERT_TRUE(result.has_value()) << result.error();
  EXPECT_EQ(result->charset, Charset::Cp437);
  EXPECT_TRUE(result->alert.empty());
  ASSERT_EQ(result->lines.size(), 1u);
  EXPECT_EQ(first_field(result->lines[0]), "\xc3\x96rebro f\xc3\xb6rening");
}

TEST(SieLexBytes, AsciiIsReadAsCp437) {
  auto const result = lex_bytes(as_bytes("#FLAGGA 0\r\n#FORMAT PC8\r\n"));
  ASSERT_TRUE(result.has_value()) << result.error();
  EXPECT_EQ(result->charset, Charset::Cp437);
  EXPECT_TRUE(result->alert.empty());
  EXPECT_EQ(result->lines.size(), 2u);
}

TEST(SieLexBytes, Utf8IsReadAsUtf8WithAnAlert) {
  // UTF-8 Ö (C3 96) is CP437 "├û"
  auto const result = lex_bytes(as_bytes("#FLAGGA 0\r\n#FNAMN \"\xc3\x96rebro\"\r\n"));
  ASSERT_TRUE(result.has_value()) << result.error();
  EXPECT_EQ(result->charset, Charset::Utf8);
  EXPECT_EQ(result->alert,
            "Not in PC8 (CP437) as SIE requires: read as CP437 it has '\xe2\x94\x9c' (U+251C) on line 2. "
            "Read as UTF-8 instead.");
  ASSERT_EQ(result->lines.size(), 2u);
  EXPECT_EQ(first_field(result->lines[1]), "\xc3\x96rebro");
}

TEST(SieLexBytes, Utf8ByteOrderMarkIsSkipped) {
  auto const result = lex_bytes(as_bytes("\xef\xbb\xbf#FNAMN \"\xc3\x96rebro\"\r\n"));
  ASSERT_TRUE(result.has_value()) << result.error();
  EXPECT_EQ(result->charset, Charset::Utf8);
  EXPECT_EQ(result->alert,
            "Not in PC8 (CP437) as SIE requires: read as CP437 it has '\xe2\x88\xa9' (U+2229) on line 1. "
            "Read as UTF-8 instead.");
  ASSERT_EQ(result->lines.size(), 1u);
  EXPECT_EQ(result->lines[0]->label, "FNAMN");
  EXPECT_EQ(first_field(result->lines[0]), "\xc3\x96rebro");
}

TEST(SieLexBytes, OtherEncodingIsAnError) {
  // Windows-1252 å (E5) is CP437 "σ" and not UTF-8
  auto const result = lex_bytes(as_bytes("#FNAMN \"Sm\xe5 AB\"\r\n"));
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Unknown encoding: not PC8 (CP437) as SIE requires (read as CP437 it has '\xcf\x83' "
                            "(U+03C3) on line 1) and not UTF-8");
}

TEST(SieLexBytes, ControlCharacterIsShownByNumberOnly) {
  auto const result = lex_bytes(as_bytes("#FNAMN \"A\x1b B\"\r\n"));
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(),
            "Unknown encoding: not PC8 (CP437) as SIE requires (read as CP437 it has (U+001B) on line 1) and not UTF-8");
}

TEST_F(SieFileTest, Cp437FileLogsItsEncoding) {
  auto const file = write("cp437.se", "#FNAMN \"\x99rebro\"\r\n");
  auto const result = lex_file(file);
  ASSERT_TRUE(result.has_value()) << result.error();
  EXPECT_EQ(result->charset, Charset::Cp437);
  EXPECT_TRUE(result->alert.empty());
  auto const text = read_log();
  EXPECT_NE(text.find("BUSINESS"), std::string::npos);
  EXPECT_NE(text.find(std::format("Read SIE file {} as CP437", file.string())), std::string::npos) << text;
}

TEST_F(SieFileTest, Utf8FileAlertsAndLogs) {
  auto const file = write("utf8.se", "#FNAMN \"\xc3\x96rebro\"\r\n");
  auto const result = lex_file(file);
  ASSERT_TRUE(result.has_value()) << result.error();
  EXPECT_EQ(result->charset, Charset::Utf8);
  EXPECT_EQ(result->alert.rfind(file.string() + ": Not in PC8 (CP437)", 0), 0u) << result->alert;
  auto const text = read_log();
  EXPECT_NE(text.find(std::format("Read SIE file {} as UTF-8", file.string())), std::string::npos) << text;
  EXPECT_NE(text.find(result->alert), std::string::npos) << text;
}

TEST_F(SieFileTest, UnknownEncodingIsADesignInsufficiency) {
  auto const file = write("cp1252.se", "#FNAMN \"Sm\xe5\"\r\n");
  auto const result = lex_file(file);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().rfind(file.string() + ": Unknown encoding", 0), 0u) << result.error();
  auto const text = read_log();
  EXPECT_NE(text.find("DESIGN_INSUFFICIENCY"), std::string::npos) << text;
  EXPECT_NE(text.find("Unknown encoding"), std::string::npos) << text;
}

TEST_F(SieFileTest, MissingFileIsAnError) {
  auto const result = lex_file(root / "none.se");
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), std::format("Cannot read {}", (root / "none.se").string()));
}
