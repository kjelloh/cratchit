#include "core/path_value.hpp"

#include <gtest/gtest.h>

using cratchit::find_value;
using cratchit::parse_path_values;

TEST(PathValue, ParsesLinesInOrder) {
  auto const entries = parse_path_values("created = 2026-10-20\nruntime = .\n");
  ASSERT_TRUE(entries.has_value()) << entries.error();
  ASSERT_EQ(entries->size(), 2u);
  EXPECT_EQ((*entries)[0].path, "created");
  EXPECT_EQ((*entries)[0].value, "2026-10-20");
  EXPECT_EQ((*entries)[1].path, "runtime");
  EXPECT_EQ((*entries)[1].value, ".");
}

TEST(PathValue, TrimsAndKeepsInnerSpaces) {
  auto const entries = parse_path_values("  runtime=  /My Books/2026 \r\n");
  ASSERT_TRUE(entries.has_value()) << entries.error();
  EXPECT_EQ(find_value(*entries, "runtime"), "/My Books/2026");
}

TEST(PathValue, SkipsBlankLinesAndAllowsNoFinalNewline) {
  auto const entries = parse_path_values("\n  \na = 1\n\nb = 2");
  ASSERT_TRUE(entries.has_value()) << entries.error();
  EXPECT_EQ(entries->size(), 2u);
  EXPECT_EQ(find_value(*entries, "b"), "2");
}

TEST(PathValue, ValueMayContainEquals) {
  auto const entries = parse_path_values("a = b = c\n");
  ASSERT_TRUE(entries.has_value()) << entries.error();
  EXPECT_EQ(find_value(*entries, "a"), "b = c");
}

TEST(PathValue, EmptyValueIsAllowed) {
  auto const entries = parse_path_values("a =\n");
  ASSERT_TRUE(entries.has_value()) << entries.error();
  EXPECT_EQ(find_value(*entries, "a"), "");
}

TEST(PathValue, LineWithoutEqualsIsAnError) {
  auto const entries = parse_path_values("a = 1\nnonsense\n");
  ASSERT_FALSE(entries.has_value());
  EXPECT_NE(entries.error().find("rad 2"), std::string::npos);
}

TEST(PathValue, EmptyPathIsAnError) {
  EXPECT_FALSE(parse_path_values(" = 1\n").has_value());
}

TEST(PathValue, RepeatedPathIsAnError) {
  EXPECT_FALSE(parse_path_values("a = 1\na = 2\n").has_value());
}

TEST(PathValue, MissingPathHasNoValue) {
  auto const entries = parse_path_values("a = 1\n");
  ASSERT_TRUE(entries.has_value()) << entries.error();
  EXPECT_FALSE(find_value(*entries, "b").has_value());
}
