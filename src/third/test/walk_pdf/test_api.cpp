#include "walk_pdf.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

using walk_pdf::extract_text;
using walk_pdf::TextError;

namespace {

std::string stream_object(int number, std::string_view dictionary, std::string_view data) {
  return std::to_string(number) + " 0 obj\n<< " + std::string{dictionary} + " /Length " +
         std::to_string(data.size()) + " >>\nstream\n" + std::string{data} + "\nendstream\nendobj\n";
}

// A one-page PDF drawing content with F1 (WinAnsi, all glyphs 500 wide) and
// F2 (Type0 without /ToUnicode: its codes can not be mapped)
std::string pdf(std::string_view content, std::string_view extra_objects = "", std::string_view trailer_extra = "") {
  std::string widths;
  for (int i = 0; i < 224; ++i) {
    widths += "500 ";
  }
  return std::string{"%PDF-1.4\n"
                     "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n"
                     "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n"
                     "3 0 obj\n<< /Type /Page /Parent 2 0 R /Contents 4 0 R"
                     " /Resources << /Font << /F1 5 0 R /F2 6 0 R >> >> >>\nendobj\n"} +
         stream_object(4, "", content) +
         "5 0 obj\n<< /Type /Font /Subtype /Type1 /Encoding /WinAnsiEncoding /FirstChar 32 /Widths [" + widths +
         "] >>\nendobj\n"
         "6 0 obj\n<< /Type /Font /Subtype /Type0 /Encoding /Identity-H >>\nendobj\n" +
         std::string{extra_objects} + "trailer\n<< /Size 7 /Root 1 0 R " + std::string{trailer_extra} + ">>\n%%EOF\n";
}

// The encryption of test_security.cpp's 128-bit vectors, with the given /P and /U
std::string encrypted(std::string_view permissions, std::string_view user) {
  return pdf("BT /F1 10 Tf 72 700 Td (x) Tj ET",
             "7 0 obj\n<< /Filter /Standard /V 2 /R 3 /Length 128 /P " + std::string{permissions} +
                 " /O <000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f> /U <" + std::string{user} +
                 "> >>\nendobj\n",
             "/Encrypt 7 0 R /ID [<00112233445566778899AABBCCDDEEFF> <00112233445566778899AABBCCDDEEFF>]");
}

// /U values that the empty password gives, computed independently (Python's
// hashlib) for /P -572 (copying not permitted) and /P -4 (everything permitted)
std::string const user_for_p572 = "1027fa8a57cfcd498f1678b74ecf23f800000000000000000000000000000000";
std::string const user_for_p4 = "83cc5b62e7379e6d8f5ce29fc8a507b000000000000000000000000000000000";

} // namespace

TEST(Api, TextAndReport) {
  auto const result = extract_text(pdf("BT /F1 10 Tf 72 700 Td (Faktura 123) Tj 0 -12 Td (Summa 492,00) Tj ET"));
  ASSERT_TRUE(result) << result.error().message;
  ASSERT_EQ(result->text.pages.size(), 1U);
  EXPECT_EQ(result->text.pages[0], (std::vector<std::string>{"Faktura 123", "Summa 492,00"}));
  EXPECT_EQ(result->report.pages, 1U);
  EXPECT_EQ(result->report.unmapped_glyphs, 0U);
  EXPECT_FALSE(result->report.encrypted);
}

TEST(Api, UnmappedGlyphsAreCounted) {
  auto const result = extract_text(pdf("BT /F2 10 Tf 72 700 Td <00010002> Tj ET"));
  ASSERT_TRUE(result);
  EXPECT_EQ(result->report.unmapped_glyphs, 2U);
}

TEST(Api, NoPagesFound) {
  auto const result = extract_text(std::string{"%PDF-1.5\n1 0 obj\n<< /Type /ObjStm /N 0 /First 0 /Length 0 >>\n"
                                               "stream\n\nendstream\nendobj\n%%EOF\n"});
  ASSERT_TRUE(result);
  EXPECT_EQ(result->report.pages, 0U);
  EXPECT_TRUE(result->text.pages.empty());
}

TEST(Api, NotAPdf) {
  auto const result = extract_text(std::string{"Hello, this is plain text"});
  ASSERT_FALSE(result);
  EXPECT_EQ(result.error().kind, TextError::Kind::not_a_pdf);
}

TEST(Api, HeaderWithinTheFirst1024Bytes) {
  EXPECT_TRUE(extract_text(std::string(100, ' ') + pdf("BT /F1 10 Tf (x) Tj ET")));
}

TEST(Api, EncryptedAndPermitted) {
  // /P -4: everything permitted
  auto const result = extract_text(encrypted("-4", user_for_p4));
  ASSERT_TRUE(result) << result.error().message;
  EXPECT_TRUE(result->report.encrypted);
}

TEST(Api, NotPermitted) {
  auto const result = extract_text(encrypted("-572", user_for_p572));
  ASSERT_FALSE(result);
  EXPECT_EQ(result.error().kind, TextError::Kind::not_permitted);
}

TEST(Api, NeedsPassword) {
  auto const result = extract_text(encrypted("-4", std::string(64, '0')));
  ASSERT_FALSE(result);
  EXPECT_EQ(result.error().kind, TextError::Kind::needs_password);
}

TEST(Api, UnsupportedEncryption) {
  auto text = encrypted("-4", user_for_p4);
  text.replace(text.find("/V 2 /R 3"), 9, "/V 5 /R 6");
  auto const result = extract_text(text);
  ASSERT_FALSE(result);
  EXPECT_EQ(result.error().kind, TextError::Kind::unsupported_encryption);
}

TEST(Api, FromAFile) {
  auto const file = std::filesystem::temp_directory_path() / "walk_pdf_test_api.pdf";
  std::ofstream{file, std::ios::binary} << pdf("BT /F1 10 Tf 72 700 Td (from a file) Tj ET");
  auto const result = extract_text(file);
  std::filesystem::remove(file);
  ASSERT_TRUE(result);
  EXPECT_EQ(result->text.pages[0], (std::vector<std::string>{"from a file"}));
}

TEST(Api, CannotRead) {
  auto const result = extract_text(std::filesystem::path{"/no/such/file.pdf"});
  ASSERT_FALSE(result);
  EXPECT_EQ(result.error().kind, TextError::Kind::cannot_read);
}
