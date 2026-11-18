#include "cmap.hpp"
#include "document.hpp"
#include "font.hpp"
#include "glyph_names.hpp"
#include "peek.hpp"

#include <gtest/gtest.h>

#include <string>

using walk_pdf::CMap;
using walk_pdf::Document;
using walk_pdf::Path;
using namespace std::string_view_literals;

TEST(GlyphNames, Names) {
  EXPECT_EQ(walk_pdf::glyph_unicode("A"), U'A');
  EXPECT_EQ(walk_pdf::glyph_unicode("adieresis"), U'ä');
  EXPECT_EQ(walk_pdf::glyph_unicode("aring"), U'å');
  EXPECT_EQ(walk_pdf::glyph_unicode("space"), U' ');
  EXPECT_EQ(walk_pdf::glyph_unicode("quoteright"), U'’');
  EXPECT_EQ(walk_pdf::glyph_unicode("Euro"), U'€');
  EXPECT_EQ(walk_pdf::glyph_unicode("fi"), U'ﬁ');
  EXPECT_EQ(walk_pdf::glyph_unicode("uni00F6"), U'ö');
  EXPECT_EQ(walk_pdf::glyph_unicode("u1F600"), U'\U0001F600');
  EXPECT_EQ(walk_pdf::glyph_unicode("a.sc"), U'a');
  EXPECT_FALSE(walk_pdf::glyph_unicode("g42"));
}

TEST(CMap, BfcharAndBfrange) {
  auto const cmap = CMap::parse("/CIDInit /ProcSet findresource begin 12 dict begin begincmap\n"
                                "1 begincodespacerange <0000> <FFFF> endcodespacerange\n"
                                "2 beginbfchar <0003> <0048> <0004> <00E4> endbfchar\n"
                                "2 beginbfrange <0010> <0012> <0061>\n"
                                "<0020> <0021> [<0066006C> <D83DDE00>] endbfrange\n"
                                "endcmap CMapName currentdict /CMap defineresource pop end end");
  EXPECT_EQ(cmap.utf8("\x00\x03"sv), "H");
  EXPECT_EQ(cmap.utf8("\x00\x04"sv), "\xC3\xA4");             // ä
  EXPECT_EQ(cmap.utf8("\x00\x10"sv), "a");
  EXPECT_EQ(cmap.utf8("\x00\x12"sv), "c");                    // a + 2
  EXPECT_EQ(cmap.utf8("\x00\x20"sv), "fl");                   // two characters for one code
  EXPECT_EQ(cmap.utf8("\x00\x21"sv), "\xF0\x9F\x98\x80");     // a surrogate pair
  EXPECT_FALSE(cmap.utf8("\x00\x13"sv));
  EXPECT_EQ(cmap.code_length("\x00\x03\x00"sv), 2U);
}

TEST(CMap, CodeSpaceOfMixedLengths) {
  // One-byte codes 00-80, two-byte codes 8140-9FFC (each byte in its own range)
  auto const cmap = CMap::parse("begincodespacerange <00> <80> <8140> <9FFC> endcodespacerange");
  EXPECT_EQ(cmap.code_length("A"), 1U);
  EXPECT_EQ(cmap.code_length("\x81\x40"sv), 2U);
  EXPECT_EQ(cmap.code_length("\x9F\xFC"sv), 2U);
  EXPECT_FALSE(cmap.code_length("\x81\x20"sv)); // second byte outside 40-FC
}

TEST(CMap, BfcharWithAGlyphName) {
  auto const cmap = CMap::parse("1 beginbfchar <01> /adieresis endbfchar");
  EXPECT_EQ(cmap.utf8("\x01"sv), "\xC3\xA4");
}

namespace {

std::string stream_object(int number, std::string_view dictionary, std::string_view data) {
  return std::to_string(number) + " 0 obj\n<< " + std::string{dictionary} + " /Length " +
         std::to_string(data.size()) + " >>\nstream\n" + std::string{data} + "\nendstream\nendobj\n";
}

// Page 2 draws with content stream 3 and fonts:
//  F1 (4): Type0, Identity-H, /ToUnicode 5 (0001 H, 0002 e, 0003-0004 l o)
//  F2 (6): Type1, WinAnsiEncoding
//  F3 (7): Type1, StandardEncoding with /Differences: 65 is ä, 66 ö
std::string const content =
    "BT /F1 12 Tf <00010002000300030004> Tj ET\n"
    "q BT /F2 10 Tf (Malm\\366 ) Tj ET Q\n"
    "BT /F2 10 Tf [(Hej) -300 (d\\345)] TJ 0 -12 Td (rad 2) Tj T* (rad 3) Tj ET\n"
    "q BT /F3 9 Tf (A) Tj ET Q BT (A) Tj ET\n"
    "BT /F3 9 Tf (AB) Tj ET";

std::string const pdf =
    "%PDF-1.4\n"
    "1 0 obj\n<< /Type /Pages /Kids [2 0 R] /Count 1 >>\nendobj\n"
    "2 0 obj\n<< /Type /Page /Parent 1 0 R /Contents 3 0 R"
    " /Resources << /Font << /F1 4 0 R /F2 6 0 R /F3 7 0 R >> >> >>\nendobj\n" +
    stream_object(3, "", content) +
    "4 0 obj\n<< /Type /Font /Subtype /Type0 /BaseFont /X /Encoding /Identity-H /ToUnicode 5 0 R >>\nendobj\n" +
    stream_object(5, "", "begincmap 1 begincodespacerange <0000> <FFFF> endcodespacerange\n"
                         "2 beginbfchar <0001> <0048> <0002> <0065> endbfchar\n"
                         "1 beginbfrange <0003> <0004> [<006C> <006F>] endbfrange endcmap") +
    "6 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>\nendobj\n"
    "7 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Y"
    " /Encoding << /BaseEncoding /StandardEncoding /Differences [65 /adieresis /odieresis] >> >>\nendobj\n"
    "%%EOF\n";

std::string decoded(int block) { return "/rev[0]/obj[3]/stream/decoded/BT[" + std::to_string(block) + "]"; }

} // namespace

TEST(Fonts, Type0WithToUnicode) {
  Document const document{pdf};
  EXPECT_EQ(document.text(*Path::parse(decoded(0))), "Hello");
  EXPECT_EQ(document.text(*Path::parse(decoded(0) + "/op[1]")), "Hello");
}

TEST(Fonts, WinAnsi) {
  Document const document{pdf};
  EXPECT_EQ(document.text(*Path::parse(decoded(1))), "Malm\xC3\xB6 ");
}

TEST(Fonts, GapsAndNewLinesAreSpaces) {
  Document const document{pdf};
  // A TJ gap of -300 and the Td that moves down and the T* are spaces
  EXPECT_EQ(document.text(*Path::parse(decoded(2))), "Hej d\xC3\xA5 rad 2 rad 3");
}

TEST(Fonts, Differences) {
  Document const document{pdf};
  EXPECT_EQ(document.text(*Path::parse(decoded(3))), "\xC3\xA4");
  EXPECT_EQ(document.text(*Path::parse(decoded(5))), "\xC3\xA4\xC3\xB6");
}

TEST(Fonts, FontRestoredByQ) {
  // "q ... /F3 Tf ... Q" restores F2: code 65 is A again, not F3's ä
  Document const document{pdf};
  EXPECT_EQ(document.text(*Path::parse(decoded(3))), "\xC3\xA4");
  EXPECT_EQ(document.text(*Path::parse(decoded(4))), "A");
}

TEST(Fonts, NoFontYet) {
  Document const document{"%PDF-1.4\n1 0 obj\n<< /Type /Page /Contents 2 0 R /Resources << >> >>\nendobj\n" +
                          stream_object(2, "", "BT (x) Tj ET") + "%%EOF\n"};
  EXPECT_FALSE(document.text(*Path::parse("/rev[0]/obj[2]/stream/decoded/BT[0]")));
}

TEST(Fonts, Peek) {
  Document const document{pdf};
  EXPECT_EQ(walk_pdf::peek(*document.find(*Path::parse(decoded(0)))), "\"Hello\"");
  EXPECT_EQ(walk_pdf::peek(*document.find(*Path::parse(decoded(0) + "/op[1]"))), "Tj \"Hello\"");
  EXPECT_EQ(walk_pdf::peek(*document.find(*Path::parse(decoded(2) + "/op[1]"))), "TJ \"Hej d\xC3\xA5\"");
}

TEST(Fonts, UnmappedCodeIsReplacementCharacter) {
  Document const document{pdf};
  auto const font = document.load_font(*Path::parse("/rev[0]/obj[4]"));
  EXPECT_EQ(font->text("\x00\x01\x00\x09"sv), "H\xEF\xBF\xBD");
  EXPECT_EQ(font->code_bytes(), 2U);
}
