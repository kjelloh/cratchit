#include "document.hpp"
#include "peek.hpp"

#include <gtest/gtest.h>

#include <string>

using walk_pdf::Document;
using walk_pdf::Path;

namespace {

std::string stream_object(int number, std::string_view dictionary, std::string_view data) {
  return std::to_string(number) + " 0 obj\n<< " + std::string{dictionary} + " /Length " +
         std::to_string(data.size()) + " >>\nstream\n" + std::string{data} + "\nendstream\nendobj\n";
}

// A page tree: pages 2 holds the resources (fonts F1 and F2); page 3
// inherits them and draws with two content streams; form 7 has its own
// resources (font F1 is object 9 there). Revision 1 updates font 5.
std::string const pdf =
    "%PDF-1.4\n"
    "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n"
    "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 /Resources << /Font << /F1 5 0 R /F2 6 0 R >> >> >>\nendobj\n"
    "3 0 obj\n<< /Type /Page /Parent 2 0 R /Contents [4 0 R 8 0 R] >>\nendobj\n" +
    stream_object(4, "", "BT /F1 12 Tf (Hi) Tj ET") +
    "5 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>\nendobj\n"
    "6 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Courier >>\nendobj\n" +
    stream_object(7, "/Type /XObject /Subtype /Form /Resources << /Font << /F1 9 0 R >> >>", "BT /F1 9 Tf (in form) Tj ET") +
    stream_object(8, "", "BT /F2 10 Tf (Second) Tj /F9 10 Tf (No such font) Tj ET") +
    "9 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Times-Roman >>\nendobj\n"
    "trailer\n<< /Size 10 /Root 1 0 R >>\n%%EOF\n"
    "5 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold >>\nendobj\n"
    "trailer\n<< /Size 10 /Root 1 0 R /Prev 0 >>\n%%EOF\n";

Path path(std::string_view text) { return *Path::parse(text); }

std::string peek_at(Document const& document, std::string_view text) {
  auto const content = document.find(path(text));
  EXPECT_TRUE(content) << text;
  return content ? walk_pdf::peek(*content) : std::string{};
}

} // namespace

TEST(References, ObjectIsItsLatestVersion) {
  Document const document{pdf};
  EXPECT_EQ(document.object(5), path("/rev[1]/obj[5]"));
  EXPECT_EQ(document.object(6), path("/rev[0]/obj[6]"));
  EXPECT_FALSE(document.object(42));
}

TEST(References, Resolve) {
  Document const document{pdf};
  EXPECT_EQ(document.resolve(path("/rev[0]/obj[1]/Pages")), path("/rev[0]/obj[2]"));
  EXPECT_EQ(document.resolve(path("/rev[0]/obj[3]/Contents/[1]")), path("/rev[0]/obj[8]"));
  EXPECT_EQ(document.resolve(path("/rev[0]/obj[2]/Resources/Font/F1")), path("/rev[1]/obj[5]"));
  EXPECT_FALSE(document.resolve(path("/rev[0]/obj[2]/Type"))); // not a reference
}

TEST(References, Follow) {
  Document const document{pdf};
  EXPECT_EQ(document.follow(path("/rev[0]/obj[1]/Pages")), path("/rev[0]/obj[2]"));
  EXPECT_EQ(document.follow(path("/rev[0]/obj[2]/Resources")), path("/rev[0]/obj[2]/Resources"));
}

TEST(References, ObjectsAreFoundWithoutScanning) {
  Document const document{pdf};
  EXPECT_EQ(document.find(path("/rev[1]/obj[5]/BaseFont"))->bytes(), "/Helvetica-Bold");
  EXPECT_EQ(document.find(path("/rev[0]/obj[5]/BaseFont"))->bytes(), "/Helvetica");
  EXPECT_FALSE(document.find(path("/rev[1]/obj[6]")));
}

TEST(References, ResourcesInheritedFromThePageTree) {
  Document const document{pdf};
  EXPECT_EQ(document.resources(path("/rev[0]/obj[4]/stream/decoded/BT[0]")), path("/rev[0]/obj[2]/Resources"));
  EXPECT_EQ(document.resources(path("/rev[0]/obj[8]")), path("/rev[0]/obj[2]/Resources"));
}

TEST(References, FormHasItsOwnResources) {
  Document const document{pdf};
  EXPECT_EQ(document.resources(path("/rev[0]/obj[7]")), path("/rev[0]/obj[7]/Resources"));
  EXPECT_EQ(document.font(path("/rev[0]/obj[7]/stream/decoded/BT[0]"), "F1"), path("/rev[0]/obj[9]"));
}

TEST(References, FontOfATextBlock) {
  Document const document{pdf};
  EXPECT_EQ(document.font(path("/rev[0]/obj[4]/stream/decoded/BT[0]/op[0]"), "F1"), path("/rev[1]/obj[5]"));
  EXPECT_EQ(document.font(path("/rev[0]/obj[8]/stream/decoded/BT[0]"), "F2"), path("/rev[0]/obj[6]"));
  EXPECT_FALSE(document.font(path("/rev[0]/obj[8]/stream/decoded/BT[0]"), "F9"));
  EXPECT_FALSE(document.resources(path("/rev[0]/obj[6]"))); // a font, not a content stream
}

TEST(References, Peek) {
  Document const document{pdf};
  EXPECT_EQ(peek_at(document, "/rev[0]/obj[3]/Parent"), "2 0 R -> /rev[0]/obj[2]");
  EXPECT_EQ(peek_at(document, "/rev[0]/obj[2]/Kids/[0]"), "3 0 R -> /rev[0]/obj[3]");
  EXPECT_EQ(peek_at(document, "/rev[0]/obj[4]/stream/decoded/BT[0]/op[0]"), "/F1 12 Tf -> /rev[1]/obj[5]");
  EXPECT_EQ(peek_at(document, "/rev[0]/obj[8]/stream/decoded/BT[0]/op[2]"), "/F9 10 Tf -> font not found");
}

TEST(References, ParentCycleEnds) {
  // Page 1 and pages 2 are each other's parent, and neither has resources
  Document const document{"%PDF-1.4\n1 0 obj\n<< /Type /Page /Parent 2 0 R /Contents 3 0 R >>\nendobj\n"
                          "2 0 obj\n<< /Type /Pages /Parent 1 0 R >>\nendobj\n" +
                          stream_object(3, "", "BT /F1 1 Tf (x) Tj ET") + "%%EOF\n"};
  EXPECT_FALSE(document.resources(path("/rev[0]/obj[3]")));
  EXPECT_EQ(peek_at(document, "/rev[0]/obj[3]/stream/decoded/BT[0]/op[0]"), "/F1 1 Tf -> font not found");
}

TEST(References, FollowEndsAtTheObject) {
  // Object 2's value is itself a reference; follow stops at object 2, as
  // references lead to objects, not to their values
  Document const document{"%PDF-1.4\n1 0 obj\n<< /A 2 0 R >>\nendobj\n2 0 obj\n1 0 R\nendobj\n%%EOF\n"};
  EXPECT_EQ(document.follow(path("/rev[0]/obj[1]/A")), path("/rev[0]/obj[2]"));
}
