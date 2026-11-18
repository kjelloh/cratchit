#include "text_string.hpp"

#include <gtest/gtest.h>

#include <string>
#include <string_view>

using walk_pdf::string_bytes;
using walk_pdf::text_to_utf8;
using namespace std::string_view_literals;

TEST(TextString, LiteralStringEscapes) {
  EXPECT_EQ(string_bytes("(plain)"), "plain");
  EXPECT_EQ(string_bytes(R"((a\nb\rc\td\be\ff))"), "a\nb\rc\td\be\ff");
  EXPECT_EQ(string_bytes(R"((\(paren\) and \\))"), R"((paren) and \)");
  EXPECT_EQ(string_bytes(R"((nested (parens) kept))"), "nested (parens) kept");
  EXPECT_EQ(string_bytes(R"((\q))"), "q"); // an unknown escape is the byte itself
}

TEST(TextString, OctalEscapes) {
  EXPECT_EQ(string_bytes(R"((\101\102C))"), "ABC");
  EXPECT_EQ(string_bytes(R"((\0053))"), "\x05" "3"); // at most three digits
  EXPECT_EQ(string_bytes(R"((\7x))"), "\x07x");
  EXPECT_EQ(string_bytes(R"((\366))"), "\xF6");
  EXPECT_EQ(string_bytes(R"((\376\377))"), "\xFE\xFF");
}

TEST(TextString, EndsOfLine) {
  EXPECT_EQ(string_bytes("(a\r\nb\rc\nd)"), "a\nb\nc\nd"); // each end of line is one LF
  EXPECT_EQ(string_bytes("(joined \\\r\nline)"), "joined line");
  EXPECT_EQ(string_bytes("(joined \\\nline)"), "joined line");
}

TEST(TextString, HexStrings) {
  EXPECT_EQ(string_bytes("<48656C6C6F>"), "Hello");
  EXPECT_EQ(string_bytes("<48 65\n6c 6C 6f>"), "Hello");
  EXPECT_EQ(string_bytes("<901FA>"), "\x90\x1F\xA0"); // an odd last digit is followed by 0
  EXPECT_EQ(string_bytes("<>"), "");
}

TEST(TextString, Utf16BigEndianText) {
  EXPECT_EQ(text_to_utf8("\xFE\xFF\0I\0n\0v\0o\0i\0c\0e"sv), "Invoice");
  EXPECT_EQ(text_to_utf8("\xFE\xFF\0M\0a\0l\0m\0\xF6"sv), "Malm\xC3\xB6");
  EXPECT_EQ(text_to_utf8("\xFE\xFF\xD8\x3D\xDE\x00"sv), "\xF0\x9F\x98\x80"); // 😀
  EXPECT_EQ(text_to_utf8("\xFE\xFF"sv), "");
  EXPECT_EQ(text_to_utf8("\xFE\xFF\0A\0"sv), "A\xEF\xBF\xBD"); // odd byte: U+FFFD
}

TEST(TextString, Utf8Text) {
  EXPECT_EQ(text_to_utf8("\xEF\xBB\xBFMalm\xC3\xB6"sv), "Malm\xC3\xB6");
  EXPECT_EQ(text_to_utf8("\xEF\xBB\xBF\xC3"sv), "\xEF\xBF\xBD"); // unfinished: U+FFFD
}

TEST(TextString, UnmarkedTextIsNotDecoded) {
  EXPECT_FALSE(text_to_utf8("Invoice"));
  EXPECT_FALSE(text_to_utf8(""));
  EXPECT_FALSE(text_to_utf8("\xFF\xFE\0A"sv)); // little-endian mark: not PDF
}

TEST(TextString, FromTokenToText) {
  EXPECT_EQ(text_to_utf8(string_bytes(R"((\376\377\000H\000i))")), "Hi");
  EXPECT_EQ(text_to_utf8(string_bytes("<FEFF00480069>")), "Hi");
}
