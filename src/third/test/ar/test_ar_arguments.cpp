#include "core/ar/arguments.hpp"

#include <gtest/gtest.h>

#include <string_view>
#include <vector>

using cratchit::ar::parse_arguments;

namespace {

auto parsed(std::vector<std::string_view> const& arguments) {
  return parse_arguments(arguments);
}

} // namespace

TEST(ArArguments, FileOnly) {
  auto const arguments = parsed({"books.se"});
  ASSERT_TRUE(arguments) << arguments.error();
  EXPECT_FALSE(arguments->help);
  EXPECT_FALSE(arguments->trace);
  EXPECT_FALSE(arguments->bas_year);
  EXPECT_EQ(arguments->file, "books.se");
}

TEST(ArArguments, OptionsInAnyOrder) {
  for (auto const& line : std::vector<std::vector<std::string_view>>{{"--trace", "--bas-year", "2026", "books.se"},
                                                                     {"--bas-year", "2026", "--trace", "books.se"},
                                                                     {"books.se", "--bas-year", "2026", "--trace"}}) {
    auto const arguments = parsed(line);
    ASSERT_TRUE(arguments) << arguments.error();
    EXPECT_TRUE(arguments->trace);
    EXPECT_EQ(arguments->bas_year, 2026);
    EXPECT_EQ(arguments->file, "books.se");
  }
}

TEST(ArArguments, Help) {
  EXPECT_TRUE(parsed({"--help"})->help);
  EXPECT_TRUE(parsed({"-h"})->help);
  EXPECT_FALSE(parsed({"--help", "books.se"}));
}

TEST(ArArguments, Errors) {
  EXPECT_EQ(parsed({}).error(), "No SIE file given");
  EXPECT_EQ(parsed({"--trace"}).error(), "No SIE file given");
  EXPECT_EQ(parsed({"--bogus", "books.se"}).error(), "Unknown option --bogus");
  EXPECT_EQ(parsed({"a.se", "b.se"}).error(), "Only one SIE file, not also b.se");
  EXPECT_EQ(parsed({"--trace", "--trace", "books.se"}).error(), "--trace is given twice");
  EXPECT_EQ(parsed({"books.se", "--bas-year"}).error(), "--bas-year needs a year, e.g. --bas-year 2026");
  EXPECT_EQ(parsed({"--bas-year", "26", "books.se"}).error(), "--bas-year needs a four digit year, not \"26\"");
  EXPECT_EQ(parsed({"--bas-year", "20x6", "books.se"}).error(), "--bas-year needs a four digit year, not \"20x6\"");
  EXPECT_EQ(parsed({"--bas-year", "2024", "--bas-year", "2026", "books.se"}).error(), "--bas-year is given twice");
}
