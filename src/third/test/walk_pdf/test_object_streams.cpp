// cratchit's addition to walk_pdf: objects kept in object streams (/Type /ObjStm)
#include "document.hpp"
#include "walk_pdf.hpp"

#include <gtest/gtest.h>
#include <zlib.h>

#include <string>
#include <vector>

namespace {

std::string deflated(std::string const& data) {
  auto size = compressBound(static_cast<uLong>(data.size()));
  std::string out(size, '\0');
  compress(reinterpret_cast<Bytef*>(out.data()), &size, reinterpret_cast<Bytef const*>(data.data()),
           static_cast<uLong>(data.size()));
  out.resize(size);
  return out;
}

// A PDF 1.5 whose page tree, page and font are in an object stream (object 9):
// only the catalog (1) and the content stream (4) are in the file itself
std::string pdf_with_object_stream() {
  std::string widths;
  for (int i = 0; i < 224; ++i) widths += "500 ";
  std::vector<std::pair<int, std::string>> const kept{
      {2, "<< /Type /Pages /Kids [3 0 R] /Count 1 >>"},
      {3, "<< /Type /Page /Parent 2 0 R /Contents 4 0 R /Resources << /Font << /F1 5 0 R >> >> >>"},
      {5, "<< /Type /Font /Subtype /Type1 /Encoding /WinAnsiEncoding /FirstChar 32 /Widths [" + widths + "] >>"}};
  std::string header;
  std::string objects;
  for (auto const& [number, value] : kept) {
    header += std::to_string(number) + " " + std::to_string(objects.size()) + " ";
    objects += value + "\n";
  }
  auto const data = deflated(header + objects);
  std::string const content = "BT /F1 10 Tf 72 700 Td (Utbetalningsbesked) Tj ET";
  return "%PDF-1.5\n"
         "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n"
         "4 0 obj\n<< /Length " + std::to_string(content.size()) + " >>\nstream\n" + content +
         "\nendstream\nendobj\n"
         "9 0 obj\n<< /Type /ObjStm /N 3 /First " + std::to_string(header.size()) + " /Filter /FlateDecode /Length " +
         std::to_string(data.size()) + " >>\nstream\n" + data + "\nendstream\nendobj\n"
         "trailer\n<< /Size 10 /Root 1 0 R >>\n%%EOF\n";
}

} // namespace

TEST(ObjectStreams, TheirObjectsAreFoundByNumber) {
  walk_pdf::Document const document{pdf_with_object_stream()};
  auto const pages = document.object(2);
  ASSERT_TRUE(pages);
  EXPECT_EQ(pages->to_string(), "/rev[0]/obj[9]/stream/decoded/obj[2]");
  auto const content = document.find(*pages);
  ASSERT_TRUE(content);
  EXPECT_EQ(content->kind(), walk_pdf::Kind::object);
  EXPECT_FALSE(content->in_file());
  // Its entries, and a reference followed from inside the stream to another object in it
  auto const kids = document.find(*walk_pdf::Path::parse("/rev[0]/obj[9]/stream/decoded/obj[2]/Count"));
  ASSERT_TRUE(kids);
  EXPECT_EQ(kids->bytes(), "1");
  EXPECT_EQ(document.pages().size(), 1u);
  EXPECT_EQ(document.object_stream_objects(*walk_pdf::Path::parse("/rev[0]/obj[9]")).size(), 3u);
  EXPECT_FALSE(document.object(7)); // in neither
}

TEST(ObjectStreams, TheTextOfPagesKeptInThem) {
  auto const result = walk_pdf::extract_text(pdf_with_object_stream());
  ASSERT_TRUE(result) << result.error().message;
  EXPECT_EQ(result->report.pages, 1u);
  ASSERT_EQ(result->text.pages.size(), 1u);
  EXPECT_EQ(result->text.pages[0], (std::vector<std::string>{"Utbetalningsbesked"}));
}
