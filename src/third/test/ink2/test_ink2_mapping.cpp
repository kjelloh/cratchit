#include "core/ink2/mapping.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <set>

using cratchit::ink2::Line;
using cratchit::ink2::parse_mapping;

namespace {

std::vector<Line> built_in() {
  auto const lines = parse_mapping(cratchit::ink2::mapping_text());
  EXPECT_TRUE(lines) << (lines ? "" : lines.error());
  return lines ? *lines : std::vector<Line>{};
}

Line const* row(std::vector<Line> const& lines, std::string_view ink2) {
  auto const found = std::ranges::find(lines, ink2, &Line::ink2);
  return found == lines.end() ? nullptr : &*found;
}

} // namespace

TEST(Ink2Mapping, BuiltInHasEveryRowOfINK2R) {
  auto const lines = built_in();
  std::set<std::string> numbers;
  std::set<std::string> codes;
  for (auto const& line : lines) {
    if (line.kind == Line::Kind::row && line.form == "INK2R") {
      EXPECT_TRUE(numbers.insert(line.ink2).second) << line.ink2;
      for (auto const& field : line.fields) {
        EXPECT_TRUE(codes.insert(field.sru).second) << field.sru;
      }
    }
  }
  // 2.1-2.50 and 3.1-3.27
  EXPECT_EQ(numbers.size(), 50u + 27u);
  for (int number = 1; number <= 50; ++number) {
    EXPECT_TRUE(numbers.contains(std::format("2.{}", number))) << number;
  }
  for (int number = 1; number <= 27; ++number) {
    EXPECT_TRUE(numbers.contains(std::format("3.{}", number))) << number;
  }
  // The minus fields of 3.2, 3.12-3.15, 3.23 and 3.24
  EXPECT_EQ(codes.size(), 50u + 27u + 7u);
}

TEST(Ink2Mapping, HeadingsAndRows) {
  auto const lines = built_in();
  ASSERT_FALSE(lines.empty());
  EXPECT_EQ(lines.front().kind, Line::Kind::heading);
  EXPECT_EQ(lines.front().text, "BALANSRÄKNING");
  auto const machines = row(lines, "2.4");
  ASSERT_TRUE(machines);
  EXPECT_EQ(machines->text, "Maskiner, inventarier och övriga materiella anläggningstillgångar");
  ASSERT_EQ(machines->fields.size(), 1u);
  EXPECT_EQ(machines->fields[0].sru, "7215");
  EXPECT_EQ(machines->fields[0].bas, "12xx (exkl. 128x)");
  EXPECT_TRUE(machines->fields[0].accounts.contains("1229"));
  EXPECT_FALSE(machines->fields[0].accounts.contains("1281"));
}

TEST(Ink2Mapping, Signs) {
  auto const lines = built_in();
  auto const sign = [&](std::string_view ink2, std::size_t field) { return row(lines, ink2)->fields.at(field).sign; };
  EXPECT_EQ(sign("2.26", 0), 1);  // 7281 kassa och bank
  EXPECT_EQ(sign("2.45", 0), -1); // 7365 leverantörsskulder
  EXPECT_EQ(sign("3.1", 0), -1);  // 7410 nettoomsättning
  EXPECT_EQ(sign("3.7", 0), 1);   // 7513 övriga externa kostnader
  EXPECT_EQ(sign("3.2", 0), -1);  // 7411 +
  EXPECT_EQ(sign("3.2", 1), 1);   // 7510 -
  EXPECT_EQ(sign("3.26", 0), 1);  // 7450 vinst: a debit on 899x
  EXPECT_EQ(sign("3.27", 0), -1); // 7550 förlust: a credit on 899x
}

TEST(Ink2Mapping, PairedFields) {
  auto const lines = built_in();
  auto const shares = row(lines, "3.12");
  ASSERT_TRUE(shares);
  ASSERT_EQ(shares->fields.size(), 2u);
  EXPECT_EQ(shares->fields[0].sru, "7414");
  EXPECT_EQ(shares->fields[1].sru, "7518");
}

TEST(Ink2Mapping, INK2SRows) {
  auto const lines = built_in();
  auto const costs = row(lines, "4.3c");
  ASSERT_TRUE(costs);
  EXPECT_EQ(costs->form, "INK2S");
  EXPECT_EQ(costs->fields[0].sru, "7653");
  EXPECT_EQ(costs->fields[0].sign, 1);
  EXPECT_TRUE(costs->fields[0].accounts.contains("6992"));
  EXPECT_TRUE(costs->fields[0].accounts.contains("8423"));
  EXPECT_FALSE(costs->fields[0].accounts.contains("6991"));
  auto const income = row(lines, "4.5c");
  ASSERT_TRUE(income);
  EXPECT_EQ(income->fields[0].sign, -1);
  EXPECT_TRUE(income->fields[0].accounts.contains("8314"));
  // The heading before the INK2S rows is of INK2S
  auto const heading = std::ranges::find_if(lines, [](Line const& line) {
    return line.kind == Line::Kind::heading && line.text.starts_with("INK2S");
  });
  ASSERT_NE(heading, lines.end());
  EXPECT_EQ(heading->form, "INK2S");
  EXPECT_EQ(lines.front().form, "INK2R");
}

TEST(Ink2Mapping, Errors) {
  EXPECT_EQ(parse_mapping("bas;sru;ink2;text\n1088;7202;2.2;Förskott\n6992;7653;;\n").error(),
            "Mapping row 3: a second field of another form than the row before it");
  EXPECT_EQ(parse_mapping("bas;sru;text\n").error(), "The mapping does not start with the header bas;sru;ink2;text");
  EXPECT_EQ(parse_mapping("bas;sru;ink2;text;;\n1088;7202;2.2;Förskott;;x\n").error(),
            "Mapping row 2: text after the fourth column");
  EXPECT_EQ(parse_mapping("bas;sru;ink2;text\n1088;;2.2;Förskott\n").error(),
            "Mapping row 2: a row of the form needs bas, sru, ink2 and text; a second field bas and sru only");
  EXPECT_EQ(parse_mapping("bas;sru;ink2;text\n1088;7802;2.2;Förskott\n").error(),
            "Mapping row 2: \"7802\" is not an SRU code of INK2R or INK2S (72xx-77xx)");
  EXPECT_EQ(parse_mapping("bas;sru;ink2;text\n;;;Rubrik\n1088;7202;;\n").error(),
            "Mapping row 3: a second field without a row of the form before it");
  EXPECT_TRUE(parse_mapping("bas;sru;ink2;text\n10x;7202;2.2;Förskott\n").error().starts_with(
      "Mapping row 2: Unreadable BAS accounts"));
}
