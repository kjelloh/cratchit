#include "plain_text.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using walk_pdf::Document;
using Lines = std::vector<std::string>;

namespace {

std::string stream_object(int number, std::string_view dictionary, std::string_view data) {
  return std::to_string(number) + " 0 obj\n<< " + std::string{dictionary} + " /Length " +
         std::to_string(data.size()) + " >>\nstream\n" + std::string{data} + "\nendstream\nendobj\n";
}

// One page drawing content with font F1: every glyph 500/1000 wide (at size
// 10: 5 units), WinAnsi; F2: Type0 Identity-H, codes 0001-0002 "A" "B", CID 1
// is 1000 wide and others 500 (/DW). Form Fm draws "form" at its /Matrix.
std::string pdf(std::string_view content, std::string_view extra_objects = "", std::string_view trailer_extra = "") {
  std::string widths;
  for (int i = 0; i < 224; ++i) {
    widths += "500 ";
  }
  return std::string{"%PDF-1.4\n"
                     "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n"
                     "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n"
                     "3 0 obj\n<< /Type /Page /Parent 2 0 R /Contents 4 0 R /Resources << /Font << /F1 5 0 R "
                     "/F2 6 0 R >> /XObject << /Fm 8 0 R >> /Properties << /P1 << /ActualText (fi) >> >> >> >>\n"
                     "endobj\n"} +
         stream_object(4, "", content) +
         "5 0 obj\n<< /Type /Font /Subtype /Type1 /Encoding /WinAnsiEncoding /FirstChar 32 /Widths [" + widths +
         "] >>\nendobj\n"
         "6 0 obj\n<< /Type /Font /Subtype /Type0 /Encoding /Identity-H /DescendantFonts [7 0 R] /ToUnicode 9 0 R >>\n"
         "endobj\n"
         "7 0 obj\n<< /Type /Font /Subtype /CIDFontType2 /DW 500 /W [1 [1000]] >>\nendobj\n" +
         stream_object(8, "/Type /XObject /Subtype /Form /Matrix [1 0 0 1 100 0] /Resources << /Font << /F1 5 0 R >> >>",
                       "BT /F1 10 Tf 0 0 Td (form) Tj ET") +
         stream_object(9, "", "begincmap 1 begincodespacerange <0000> <FFFF> endcodespacerange "
                              "1 beginbfrange <0001> <0002> <0041> endbfrange endcmap") +
         std::string{extra_objects} + "trailer\n<< /Size 10 /Root 1 0 R " + std::string{trailer_extra} +
         ">>\n%%EOF\n";
}

Lines lines(std::string_view content) {
  Document const document{pdf(content)};
  auto const text = walk_pdf::plain_text(document);
  EXPECT_TRUE(text) << (text ? "" : text.error());
  return text && !text->pages.empty() ? text->pages[0] : Lines{};
}

} // namespace

TEST(PlainText, PiecesOfAWordAreJoined) {
  // "BAS-intr" is 8 glyphs, 40 units: the next piece starts where it ends
  EXPECT_EQ(lines("BT /F1 10 Tf 72 700 Td (BAS-intr) Tj 40 0 Td (essent) Tj 30 0 Td (er) Tj ET"),
            (Lines{"BAS-intressenter"}));
}

TEST(PlainText, GapIsASpace) {
  // A gap of 3 units (more than a fifth of the size 10) between the words
  EXPECT_EQ(lines("BT /F1 10 Tf 72 700 Td (Hello) Tj 28 0 Td (world) Tj ET"), (Lines{"Hello world"}));
  // A gap of 1 unit is not
  EXPECT_EQ(lines("BT /F1 10 Tf 72 700 Td (Hello) Tj 26 0 Td (world) Tj ET"), (Lines{"Helloworld"}));
}

TEST(PlainText, LinesTopToBottomWhateverTheDrawingOrder) {
  EXPECT_EQ(lines("BT /F1 10 Tf 72 688 Td (second) Tj ET BT /F1 10 Tf 72 700 Td (first) Tj ET"),
            (Lines{"first", "second"}));
}

TEST(PlainText, TextOnTheSameBaselineIsOneLine) {
  // Drawn right half first; a little lower (a subscript) still on the line
  EXPECT_EQ(lines("BT /F1 10 Tf 200 700 Td (right) Tj ET BT /F1 10 Tf 72 700 Td (left) Tj 30 -2 Td (sub) Tj ET"),
            (Lines{"left sub right"}));
}

TEST(PlainText, TjKerningAndWideGaps) {
  // -100: a small kern (1 unit), -600: a gap (6 units)
  EXPECT_EQ(lines("BT /F1 10 Tf 72 700 Td [(Ke) -100 (rn) -600 (gap)] TJ ET"), (Lines{"Kern gap"}));
}

TEST(PlainText, LeadingAndNextLine) {
  EXPECT_EQ(lines("BT /F1 10 Tf 12 TL 72 700 Td (one) Tj T* (two) Tj (three) ' ET"), (Lines{"one", "two", "three"}));
}

TEST(PlainText, EmptyLineForAWideGap) {
  EXPECT_EQ(lines("BT /F1 10 Tf 72 700 Td (one) Tj 0 -12 Td (two) Tj 0 -40 Td (three) Tj ET"),
            (Lines{"one", "two", "", "three"}));
}

TEST(PlainText, CurrentTransformationMatrixScales) {
  // Twice as large: the glyphs are 10 units wide, so a 3-unit Td gap becomes 6
  EXPECT_EQ(lines("2 0 0 2 0 0 cm BT /F1 10 Tf 10 300 Td (ab) Tj 13 0 Td (cd) Tj ET"), (Lines{"ab cd"}));
}

TEST(PlainText, SaveAndRestore) {
  // The scaling inside q ... Q does not apply after it
  EXPECT_EQ(lines("q 1 0 0 1 0 -100 cm BT /F1 10 Tf 72 700 Td (low) Tj ET Q BT /F1 10 Tf 72 700 Td (high) Tj ET"),
            (Lines{"high", "", "low"})); // 100 units apart: an empty line between
}

TEST(PlainText, FormWithMatrix) {
  // The form draws at 0 0, moved by its /Matrix to x 100: after "page" on the same line
  EXPECT_EQ(lines("1 0 0 1 0 700 cm BT /F1 10 Tf 40 0 Td (page) Tj ET /Fm Do"), (Lines{"page form"}));
}

TEST(PlainText, RotatedText) {
  // A page turned a quarter: lines run upwards, and still read as lines
  EXPECT_EQ(lines("0 1 -1 0 500 0 cm BT /F1 10 Tf 72 100 Td (first) Tj 0 -12 Td (second) Tj ET"),
            (Lines{"first", "second"}));
}

TEST(PlainText, ActualText) {
  // The marked content's /ActualText replaces the text its glyphs map to:
  // inline in the BDC, or by name in the resources' /Properties
  EXPECT_EQ(lines("BT /F1 10 Tf 72 700 Td (A) Tj /Span <</ActualText (-)>> BDC ( ) Tj EMC (B) Tj ET"),
            (Lines{"A-B"}));
  EXPECT_EQ(lines("BT /F1 10 Tf 72 700 Td (x) Tj /Span /P1 BDC (#) Tj EMC (y) Tj ET"), (Lines{"xfiy"}));
}

TEST(PlainText, Type0Widths) {
  // CID 1 ("A") is 1000 wide: 10 units; "B" (CID 2) starts right after it, then a gap
  EXPECT_EQ(lines("BT /F2 10 Tf 72 700 Td <0001> Tj 10 0 Td <0002> Tj 8 0 Td <0001> Tj ET"), (Lines{"AB A"}));
}

TEST(PlainText, TextWithoutFontIsLeftOut) {
  EXPECT_EQ(lines("BT 72 700 Td (nofont) Tj /F1 10 Tf (font) Tj ET"), (Lines{"font"}));
}

TEST(PlainText, PagesAndToString) {
  Document const document{pdf("BT /F1 10 Tf 72 700 Td (one) Tj ET")};
  auto const text = walk_pdf::plain_text(document);
  ASSERT_TRUE(text);
  EXPECT_EQ(text->to_string(), "--- page 1 ---\none\n");
}

TEST(PlainText, RefusedWhenCopyingIsNotPermitted) {
  // The encryption of test_security.cpp's 128-bit vectors: /P -572 does not permit copying
  auto const text = pdf("BT /F1 10 Tf 72 700 Td (x) Tj ET",
                        "10 0 obj\n<< /Filter /Standard /V 2 /R 3 /Length 128 /P -572 "
                        "/O <000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f> "
                        "/U <1027fa8a57cfcd498f1678b74ecf23f800000000000000000000000000000000> >>\nendobj\n",
                        "/Encrypt 10 0 R /ID [<00112233445566778899AABBCCDDEEFF> <00112233445566778899AABBCCDDEEFF>]");
  Document const document{text};
  ASSERT_TRUE(document.security());
  auto const result = walk_pdf::plain_text(document);
  ASSERT_FALSE(result);
  EXPECT_EQ(result.error(), "the file's permissions do not allow copying or extracting its text");
}
