#include "charset/cp437.hpp"
#include "core/sie/lexer.hpp"

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <vector>

using cratchit::sie::Field;
using cratchit::sie::Lexed;
using cratchit::sie::Lexer;
using cratchit::sie::Line;
using cratchit::sie::ObjectList;

namespace {

// Every line from pushing the code points one at a time, then finishing
std::vector<Lexed> lexed(std::u32string_view text) {
  Lexer lexer;
  std::vector<Lexed> result;
  for (auto const code_point : text) {
    if (auto line = lexer.push(code_point)) {
      result.push_back(std::move(*line));
    }
  }
  if (auto line = lexer.finish()) {
    result.push_back(std::move(*line));
  }
  return result;
}

// The one line of a text that must lex without error
Line one_line(std::u32string_view text) {
  auto const lines = lexed(text);
  EXPECT_EQ(lines.size(), 1u);
  if (lines.size() != 1 || !lines[0].has_value()) {
    ADD_FAILURE() << (lines.empty() || lines[0].has_value() ? "no line" : lines[0].error());
    return {};
  }
  return *lines[0];
}

// The error text of the one line of a text that must fail
std::string one_error(std::u32string_view text) {
  auto const lines = lexed(text);
  EXPECT_EQ(lines.size(), 1u);
  if (lines.size() != 1 || lines[0].has_value()) {
    ADD_FAILURE() << "expected one failed line";
    return {};
  }
  return lines[0].error();
}

Field text(std::string value) {
  return Field{std::move(value)};
}

Field list(std::vector<std::string> items) {
  return Field{ObjectList{std::move(items)}};
}

} // namespace

TEST(SieLexer, RecordWithPlainFields) {
  auto const line = one_line(U"#RAR 0 20260501 20270430\r\n");
  EXPECT_EQ(line.kind, Line::Kind::record);
  EXPECT_EQ(line.label, "RAR");
  EXPECT_EQ(line.fields, (std::vector{text("0"), text("20260501"), text("20270430")}));
  EXPECT_EQ(line.number, 1u);
}

TEST(SieLexer, RecordWithoutFields) {
  auto const line = one_line(U"#KSUMMA\n");
  EXPECT_EQ(line.label, "KSUMMA");
  EXPECT_TRUE(line.fields.empty());
}

TEST(SieLexer, QuotedFieldKeepsSpacesAndBraces) {
  auto const line = one_line(U"#FNAMN \"The ITfied {AB}\"\n");
  EXPECT_EQ(line.fields, (std::vector{text("The ITfied {AB}")}));
}

TEST(SieLexer, EmptyQuotedFieldIsAField) {
  auto const line = one_line(U"#TRANS 1930 {} -13000 \"\" \"\" 0\n");
  EXPECT_EQ(line.fields, (std::vector{text("1930"), list({}), text("-13000"), text(""), text(""), text("0")}));
}

TEST(SieLexer, EscapedQuote) {
  auto const line = one_line(U"#X \"Reflexv\u00e4st \\\"UAS-Pilot\\\"\" 0\n");
  EXPECT_EQ(line.fields, (std::vector{text("Reflexv\xc3\xa4st \"UAS-Pilot\""), text("0")}));
}

TEST(SieLexer, OtherBackslashIsKept) {
  auto const line = one_line(U"#X \"C:\\\\a\\b\"\n");
  EXPECT_EQ(line.fields, (std::vector{text("C:\\\\a\\b")}));
}

TEST(SieLexer, QuoteInsideUnquotedFieldIsKept) {
  auto const line = one_line(U"#X 1/4\" b\n");
  EXPECT_EQ(line.fields, (std::vector{text("1/4\""), text("b")}));
}

TEST(SieLexer, ObjectListItems) {
  auto const line = one_line(U"#TRANS 3010 {1 \"100\" 6 \"P 1\"} -500\n");
  EXPECT_EQ(line.fields, (std::vector{text("3010"), list({"1", "100", "6", "P 1"}), text("-500")}));
}

TEST(SieLexer, ObjectListWithSpacesInsideBraces) {
  auto const line = one_line(U"#X { 1 2 } {3}\n");
  EXPECT_EQ(line.fields, (std::vector{list({"1", "2"}), list({"3"})}));
}

TEST(SieLexer, TabsSeparateAndLeadingBlanksAreSkipped) {
  auto const line = one_line(U" \t#TRANS\t1920\t{}\t1.85\n");
  EXPECT_EQ(line.label, "TRANS");
  EXPECT_EQ(line.fields, (std::vector{text("1920"), list({}), text("1.85")}));
}

TEST(SieLexer, BlockLinesAndBlankLines) {
  auto const lines = lexed(U"#VER A 1 20260505 \"x\"\r\n{\r\n\r\n  #TRANS 1930 {} -1\r\n }  \r\n");
  ASSERT_EQ(lines.size(), 4u);
  for (auto const& line : lines) {
    ASSERT_TRUE(line.has_value()) << line.error();
  }
  EXPECT_EQ(lines[0]->label, "VER");
  EXPECT_EQ(lines[1]->kind, Line::Kind::block_begin);
  EXPECT_EQ(lines[1]->number, 2u);
  EXPECT_EQ(lines[2]->label, "TRANS");
  EXPECT_EQ(lines[2]->number, 4u);
  EXPECT_EQ(lines[3]->kind, Line::Kind::block_end);
  EXPECT_EQ(lines[3]->number, 5u);
}

TEST(SieLexer, LastLineWithoutLineFeed) {
  auto const lines = lexed(U"#FLAGGA 0\n#GEN 20261019");
  ASSERT_EQ(lines.size(), 2u);
  ASSERT_TRUE(lines[1].has_value()) << lines[1].error();
  EXPECT_EQ(lines[1]->label, "GEN");
  EXPECT_EQ(lines[1]->number, 2u);
}

TEST(SieLexer, NothingForEmptyOrBlankInput) {
  EXPECT_TRUE(lexed(U"").empty());
  EXPECT_TRUE(lexed(U" \r\n\t\n").empty());
}

TEST(SieLexer, Errors) {
  EXPECT_EQ(one_error(U"FLAGGA 0\n"), "line 1: not a record (does not start with #), { or }");
  EXPECT_EQ(one_error(U"# 0\n"), "line 1: # without a label");
  EXPECT_EQ(one_error(U"#\n"), "line 1: # without a label");
  EXPECT_EQ(one_error(U"#X \"abc\n"), "line 1: quote not closed");
  EXPECT_EQ(one_error(U"#X {1 2\n"), "line 1: object list not closed");
  EXPECT_EQ(one_error(U"#X {1 {2}}\n"), "line 1: { inside an object list");
  EXPECT_EQ(one_error(U"#X }\n"), "line 1: } outside an object list");
  EXPECT_EQ(one_error(U"#X \"a\"b\n"), "line 1: text right after a closing quote");
  EXPECT_EQ(one_error(U"{ #X\n"), "line 1: text after a lone { or }");
}

TEST(SieLexer, ContinuesAfterAnError) {
  auto const lines = lexed(U"#X \"open\n#Y 1\n");
  ASSERT_EQ(lines.size(), 2u);
  EXPECT_FALSE(lines[0].has_value());
  ASSERT_TRUE(lines[1].has_value()) << lines[1].error();
  EXPECT_EQ(lines[1]->label, "Y");
  EXPECT_EQ(lines[1]->number, 2u);
}

TEST(SieLexer, FromCp437Bytes) {
  // "Överföring" as Fortnox writes it in PC8 (CP437): Ö = 0x99, ö = 0x94
  std::string_view const bytes = "#VER A 1 20260505 \"\x99verf\x94ring\" 20260715\r\n";
  charset::cp437::Decoder decoder;
  Lexer lexer;
  std::vector<Lexed> lines;
  for (char const c : bytes) {
    if (auto const code_point = decoder.push(static_cast<charset::cp437::CodePoint>(c))) {
      if (auto line = lexer.push(*code_point)) {
        lines.push_back(std::move(*line));
      }
    }
  }
  ASSERT_EQ(lines.size(), 1u);
  ASSERT_TRUE(lines[0].has_value()) << lines[0].error();
  EXPECT_EQ(lines[0]->fields,
            (std::vector{text("A"), text("1"), text("20260505"), text("\xc3\x96verf\xc3\xb6ring"), text("20260715")}));
}
