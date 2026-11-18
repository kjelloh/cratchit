#include "core/ink2/arguments.hpp"

#include <gtest/gtest.h>

#include <filesystem>

using cratchit::ink2::parse_arguments;

TEST(Ink2Arguments, SieAndLastYearsReturn) {
  auto const arguments = parse_arguments({"books.se", "last/BLANKETTER.SRU"});
  ASSERT_TRUE(arguments) << arguments.error();
  EXPECT_FALSE(arguments->help);
  EXPECT_EQ(arguments->sie, "books.se");
  EXPECT_EQ(arguments->previous, std::filesystem::path("last/BLANKETTER.SRU"));
  EXPECT_FALSE(arguments->previous_deficit);
  // In any order, the endings in any case
  auto const turned = parse_arguments({"blanketter.sru", "BOOKS.SE"});
  ASSERT_TRUE(turned) << turned.error();
  EXPECT_EQ(turned->sie, "BOOKS.SE");
  EXPECT_TRUE(parse_arguments({"books.SIE", "x.Sru"}));
  EXPECT_TRUE(parse_arguments({"books.si", "x.sru"}));
}

TEST(Ink2Arguments, LastYearsReturnAsAFolder) {
  auto const folder = std::filesystem::temp_directory_path();
  auto const arguments = parse_arguments({folder.string(), "books.se"});
  ASSERT_TRUE(arguments) << arguments.error();
  EXPECT_EQ(arguments->previous, folder);
}

TEST(Ink2Arguments, PreviousDeficit) {
  auto const arguments = parse_arguments({"--previous-deficit", "205963", "books.se"});
  ASSERT_TRUE(arguments) << arguments.error();
  EXPECT_EQ(arguments->previous_deficit, 205963);
  EXPECT_FALSE(arguments->previous);
  EXPECT_EQ(parse_arguments({"books.se", "--previous-deficit", "0"})->previous_deficit, 0);
}

TEST(Ink2Arguments, Trace) {
  auto const arguments = parse_arguments({"a.se", "--trace", "x.sru"});
  ASSERT_TRUE(arguments) << arguments.error();
  EXPECT_TRUE(arguments->trace);
  EXPECT_FALSE(parse_arguments({"a.se", "x.sru"})->trace);
}

TEST(Ink2Arguments, Help) {
  EXPECT_TRUE(parse_arguments({"--help"})->help);
  EXPECT_TRUE(parse_arguments({"-h"})->help);
}

TEST(Ink2Arguments, Wrong) {
  EXPECT_EQ(parse_arguments({}).error(), "No SIE file given");
  EXPECT_EQ(parse_arguments({"x.sru"}).error(), "No SIE file given");
  EXPECT_EQ(parse_arguments({"a.se"}).error(),
            "Last year's return is needed for its underskott: its BLANKETTER.SRU (or folder), or --previous-deficit");
  EXPECT_EQ(parse_arguments({"a.se", "b.se", "x.sru"}).error(), "Only one SIE file, not also b.se");
  EXPECT_EQ(parse_arguments({"a.se", "x.sru", "y.sru"}).error(), "Only one of last year's returns, not also y.sru");
  EXPECT_EQ(parse_arguments({"a.se", "x.sru", "--previous-deficit", "1"}).error(),
            "Give last year's return or --previous-deficit, not both");
  EXPECT_EQ(parse_arguments({"a.se", "--previous-deficit"}).error(),
            "--previous-deficit needs last year's underskott in kronor, e.g. --previous-deficit 0");
  for (char const* const wrong : {"-1", "1.5", "1 000", "", "x"}) {
    EXPECT_EQ(parse_arguments({"a.se", "--previous-deficit", wrong}).error(),
              std::format("--previous-deficit needs whole kronor of 0 or more, without spaces, not \"{}\"", wrong));
  }
  EXPECT_EQ(parse_arguments({"a.se", "--previous-deficit", "1", "--previous-deficit", "2"}).error(),
            "--previous-deficit is given twice");
  EXPECT_EQ(parse_arguments({"--trace", "a.se", "x.sru", "--trace"}).error(), "--trace is given twice");
  EXPECT_EQ(parse_arguments({"--verbose", "a.se"}).error(), "Unknown option --verbose");
  EXPECT_EQ(parse_arguments({"--help", "a.se"}).error(), "Unknown option --help");
  EXPECT_EQ(parse_arguments({"a.se", "notes.txt"}).error(),
            "notes.txt is neither an SIE file (.se, .si, .sie) nor last year's return (.sru, or a folder of it)");
}
