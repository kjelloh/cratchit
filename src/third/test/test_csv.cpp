#include "core/csv.hpp"

#include <gtest/gtest.h>

using cratchit::CsvRow;
using cratchit::format_csv_row;
using cratchit::parse_csv;

TEST(Csv, PlainFieldsAreNotQuoted) {
  EXPECT_EQ(format_csv_row({"1930", "Bank", "", "-10.00"}), "1930,Bank,,-10.00\n");
}

TEST(Csv, SpecialFieldsAreQuoted) {
  EXPECT_EQ(format_csv_row({"a,b", "say \"hi\"", "two\nlines", " pad"}),
            "\"a,b\",\"say \"\"hi\"\"\",\"two\nlines\",\" pad\"\n");
}

TEST(Csv, RoundTrips) {
  std::vector<CsvRow> const rows{{"a,b", "say \"hi\"", "two\nlines", " pad", ""}, {"Överföring", "x"}, {""}};
  std::string text;
  for (auto const& row : rows) {
    text += format_csv_row(row);
  }
  auto const parsed = parse_csv(text);
  ASSERT_TRUE(parsed.has_value()) << parsed.error();
  // A row of one empty field is a blank line, which is skipped
  EXPECT_EQ(*parsed, (std::vector<CsvRow>{rows[0], rows[1]}));
}

TEST(Csv, CrLfAndNoFinalNewline) {
  auto const parsed = parse_csv("a,b\r\nc,d");
  ASSERT_TRUE(parsed.has_value()) << parsed.error();
  EXPECT_EQ(*parsed, (std::vector<CsvRow>{{"a", "b"}, {"c", "d"}}));
}

TEST(Csv, Errors) {
  EXPECT_EQ(parse_csv("a,\"b\n").error(), "line 2: quote not closed");
  EXPECT_EQ(parse_csv("\"a\"b,c\n").error(), "line 1: text after a closing quote");
}
