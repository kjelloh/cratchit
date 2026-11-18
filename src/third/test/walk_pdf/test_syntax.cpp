#include "syntax.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using walk_pdf::Kind;
using walk_pdf::Value;

namespace {

// The kind and text of the value at the start of text
std::pair<Kind, std::string> value_of(std::string_view text) {
  auto const value = walk_pdf::value_at(text, 0, text.size());
  EXPECT_TRUE(value) << text;
  if (!value) {
    return {Kind::unknown, {}};
  }
  return {value->kind, std::string{text.substr(value->begin, value->end - value->begin)}};
}

// "key=value" for a dictionary's entries, or "value" for an array's items
std::vector<std::string> children_of(std::string_view text) {
  auto const container = walk_pdf::value_at(text, 0, text.size());
  EXPECT_TRUE(container);
  std::vector<std::string> result;
  for (auto pos = walk_pdf::first_child_at(*container);;) {
    auto const child = walk_pdf::next_child(text, *container, pos);
    if (!child) {
      break;
    }
    pos = child->value.end;
    auto const value = std::string{text.substr(child->value.begin, child->value.end - child->value.begin)};
    result.push_back(child->key.empty() ? value : child->key + "=" + value);
  }
  return result;
}

} // namespace

TEST(Syntax, Scalars) {
  EXPECT_EQ(value_of("null"), std::pair(Kind::null, std::string{"null"}));
  EXPECT_EQ(value_of("true "), std::pair(Kind::boolean, std::string{"true"}));
  EXPECT_EQ(value_of("false]"), std::pair(Kind::boolean, std::string{"false"}));
  EXPECT_EQ(value_of("-42 "), std::pair(Kind::integer, std::string{"-42"}));
  EXPECT_EQ(value_of("+17"), std::pair(Kind::integer, std::string{"+17"}));
  EXPECT_EQ(value_of("3.14/X"), std::pair(Kind::real, std::string{"3.14"}));
  EXPECT_EQ(value_of("-.5"), std::pair(Kind::real, std::string{"-.5"}));
  EXPECT_EQ(value_of("/Type/Page"), std::pair(Kind::name, std::string{"/Type"}));
  EXPECT_EQ(value_of("/"), std::pair(Kind::name, std::string{"/"}));
  EXPECT_EQ(value_of("bogus 1"), std::pair(Kind::unknown, std::string{"bogus"}));
}

TEST(Syntax, References) {
  EXPECT_EQ(value_of("12 0 R"), std::pair(Kind::reference, std::string{"12 0 R"}));
  EXPECT_EQ(value_of("12\n0\nR>>"), std::pair(Kind::reference, std::string{"12\n0\nR"}));
  EXPECT_EQ(value_of("12 0 RG"), std::pair(Kind::integer, std::string{"12"}));
  EXPECT_EQ(value_of("12 0 ]"), std::pair(Kind::integer, std::string{"12"}));
  EXPECT_EQ(value_of("-12 0 R"), std::pair(Kind::integer, std::string{"-12"}));
}

TEST(Syntax, Strings) {
  EXPECT_EQ(value_of("(a (nested) string) x"),
            std::pair(Kind::string, std::string{"(a (nested) string)"}));
  EXPECT_EQ(value_of(R"((escaped \) paren and \\) x)"),
            std::pair(Kind::string, std::string{R"((escaped \) paren and \\))"}));
  EXPECT_EQ(value_of("(endobj >> ] inside) x"),
            std::pair(Kind::string, std::string{"(endobj >> ] inside)"}));
  EXPECT_EQ(value_of("<48656C6C6F> x"), std::pair(Kind::string, std::string{"<48656C6C6F>"}));
}

TEST(Syntax, Containers) {
  EXPECT_EQ(value_of("<< /A << /B [1 2] >> /C (>>) >> x"),
            std::pair(Kind::dictionary, std::string{"<< /A << /B [1 2] >> /C (>>) >>"}));
  EXPECT_EQ(value_of("[1 [2 3] (]) /N] x"),
            std::pair(Kind::array, std::string{"[1 [2 3] (]) /N]"}));
  EXPECT_EQ(value_of("<<>>"), std::pair(Kind::dictionary, std::string{"<<>>"}));
  EXPECT_EQ(value_of("[]"), std::pair(Kind::array, std::string{"[]"}));
}

TEST(Syntax, CommentsAreSkipped) {
  EXPECT_EQ(value_of("% a comment\n  42"), std::pair(Kind::integer, std::string{"42"}));
  EXPECT_EQ(children_of("[1 % one\n 2]"), (std::vector<std::string>{"1", "2"}));
}

TEST(Syntax, DictionaryEntries) {
  EXPECT_EQ(children_of("<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595 842] /Font << /F1 5 0 R "
                        ">> /A#20B true >>"),
            (std::vector<std::string>{"Type=/Page", "Parent=2 0 R", "MediaBox=[0 0 595 842]",
                                      "Font=<< /F1 5 0 R >>", "A B=true"}));
  EXPECT_EQ(children_of("<</Length 3/Filter/FlateDecode>>"),
            (std::vector<std::string>{"Length=3", "Filter=/FlateDecode"}));
  EXPECT_TRUE(children_of("<< >>").empty());
}

TEST(Syntax, ArrayItems) {
  EXPECT_EQ(children_of("[ 1 2.5 /N (s) <00> [ 3 ] << /K 4 >> 7 0 R null ]"),
            (std::vector<std::string>{"1", "2.5", "/N", "(s)", "<00>", "[ 3 ]", "<< /K 4 >>",
                                      "7 0 R", "null"}));
}

TEST(Syntax, DamagedContainersRunToTheEnd) {
  EXPECT_EQ(value_of("<< /A 1"), std::pair(Kind::dictionary, std::string{"<< /A 1"}));
  EXPECT_EQ(children_of("<< /A 1 /B"), (std::vector<std::string>{"A=1"}));
  EXPECT_EQ(children_of("[1 2"), (std::vector<std::string>{"1", "2"}));
  EXPECT_EQ(children_of("<< 7 /A 1 >>"), (std::vector<std::string>{"A=1"})); // not a key: skipped
}

TEST(Syntax, StreamData) {
  std::string const text = "<< /Length 3 >>\nstream\r\nabc\r\nendstream\nendobj";
  auto const dictionary = walk_pdf::value_at(text, 0, text.size());
  auto const data = walk_pdf::stream_data(text, dictionary->end, text.size());
  ASSERT_TRUE(data);
  EXPECT_EQ(text.substr(data->begin, data->end - data->begin), "abc");
  EXPECT_FALSE(walk_pdf::stream_data("<< >>\nendobj", 5, 12));
}

TEST(Syntax, DecodeName) {
  EXPECT_EQ(walk_pdf::decode_name("/Type"), "Type");
  EXPECT_EQ(walk_pdf::decode_name("/A#20B"), "A B");
  EXPECT_EQ(walk_pdf::decode_name("/A#2"), "A#2"); // not an escape
  EXPECT_EQ(walk_pdf::decode_name("/"), "");
}

namespace {

// "operands|operator" for each operation in a content stream
std::vector<std::string> operations_of(std::string_view text) {
  std::vector<std::string> result;
  for (std::size_t pos = 0;;) {
    auto const operation = walk_pdf::next_operation(text, pos, text.size());
    if (!operation) {
      break;
    }
    pos = operation->end;
    result.push_back(std::string{text.substr(operation->begin, operation->operator_begin - operation->begin)} + "|" +
                     std::string{operation->name});
  }
  return result;
}

} // namespace

TEST(Syntax, ContentStreamOperations) {
  EXPECT_EQ(operations_of("q 1 0 0 1 72 712 cm BT /F1 12 Tf (Hi) Tj T* [(A) -50 (B)] TJ ET Q"),
            (std::vector<std::string>{"|q", "1 0 0 1 72 712 |cm", "|BT", "/F1 12 |Tf", "(Hi) |Tj", "|T*",
                                      "[(A) -50 (B)] |TJ", "|ET", "|Q"}));
  EXPECT_EQ(operations_of("1 2 (x) \" (y) '"), (std::vector<std::string>{"1 2 (x) |\"", "(y) |'"}));
  EXPECT_EQ(operations_of("/P <</MCID 3>> BDC EMC"), (std::vector<std::string>{"/P <</MCID 3>> |BDC", "|EMC"}));
  EXPECT_EQ(operations_of("% comment\n0 g"), (std::vector<std::string>{"0 |g"}));
  EXPECT_TRUE(operations_of("  ").empty());
}

TEST(Syntax, InlineImage) {
  std::string const text = "q BI /W 1 /H 1 ID \x00EI\xFF EI Q";
  auto const ops = operations_of(text);
  ASSERT_EQ(ops.size(), 3U);
  EXPECT_EQ(ops[1], "|BI");
  EXPECT_EQ(ops[2], "|Q");
}
