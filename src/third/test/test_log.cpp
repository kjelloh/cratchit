#include "core/log.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
namespace logging = cratchit::log; // not "log": that name is the math function on Windows

namespace {

// A log file of its own for each test, closed and removed afterwards
class LogTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    root = fs::temp_directory_path() / (std::string("cratchit_test_") + info->name());
    fs::remove_all(root);
    fs::create_directories(root);
    file = root / "log.txt";
  }
  void TearDown() override {
    logging::close();
    fs::remove_all(root);
  }

  std::string read_log() const {
    std::ifstream in(file);
    std::ostringstream text;
    text << in.rdbuf();
    return text.str();
  }

  fs::path root;
  fs::path file;
};

} // namespace

TEST(LogLine, HasTimeKindPlaceAndMessage) {
  auto const where = std::source_location::current();
  auto const line = logging::format_line("2026-10-21T14:03:12", logging::Kind::Business, where, "Opened workspace /x");
  EXPECT_EQ(line, std::format("2026-10-21T14:03:12 BUSINESS test_log.cpp:{} Opened workspace /x", where.line()));
}

TEST(LogLine, NamesEachKind) {
  auto const where = std::source_location::current();
  EXPECT_NE(logging::format_line("t", logging::Kind::DevelopmentTrace, where, "m").find(" DEVELOPMENT_TRACE "),
            std::string::npos);
  EXPECT_NE(logging::format_line("t", logging::Kind::DesignInsufficiency, where, "m").find(" DESIGN_INSUFFICIENCY "),
            std::string::npos);
}

TEST(LogLine, KeepsAnEventOnOneLine) {
  auto const line = logging::format_line("t", logging::Kind::Business, std::source_location::current(), "a\r\nb");
  EXPECT_TRUE(line.ends_with(" a\\nb"));
}

TEST_F(LogTest, EventsBeforeOpenAreWrittenOnOpen) {
  LOG_BUSINESS("first {}", 1);
  EXPECT_FALSE(fs::exists(file));
  ASSERT_TRUE(logging::open(file));
  LOG_DESIGN_INSUFFICIENCY("second");
  auto const text = read_log();
  auto const first = text.find(" BUSINESS test_log.cpp:");
  auto const second = text.find(" DESIGN_INSUFFICIENCY test_log.cpp:");
  ASSERT_NE(first, std::string::npos);
  ASSERT_NE(second, std::string::npos);
  EXPECT_LT(first, second);
  EXPECT_NE(text.find(" first 1\n"), std::string::npos);
  EXPECT_NE(text.find(" second\n"), std::string::npos);
}

TEST_F(LogTest, AppendsToAnExistingLog) {
  std::ofstream(file) << "earlier\n";
  ASSERT_TRUE(logging::open(file));
  LOG_BUSINESS("later");
  auto const text = read_log();
  EXPECT_TRUE(text.starts_with("earlier\n"));
  EXPECT_TRUE(text.ends_with(" later\n"));
}

TEST_F(LogTest, DevelopmentTraceFollowsTheBuildFlag) {
  ASSERT_TRUE(logging::open(file));
  LOG_DEVELOPMENT_TRACE("traced");
  bool const traced = read_log().find(" DEVELOPMENT_TRACE ") != std::string::npos;
  EXPECT_EQ(traced, CRATCHIT_DEVELOPMENT_TRACE != 0);
}

TEST_F(LogTest, OpenFailsForAMissingFolder) {
  auto const opened = logging::open(root / "missing" / "log.txt");
  ASSERT_FALSE(opened);
  EXPECT_NE(opened.error().find("Kan inte öppna loggen"), std::string::npos);
}
