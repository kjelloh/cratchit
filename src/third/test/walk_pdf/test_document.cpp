#include "document.hpp"

#include <gtest/gtest.h>
#include <zlib.h>

#include <string>
#include <vector>

using walk_pdf::Document;
using walk_pdf::Kind;
using walk_pdf::Path;

namespace {

// A small PDF with one appended update, so two revisions. Object 1 is in both.
std::string const two_revisions =
    "%PDF-1.7\n"
    "%\xe2\xe3\xcf\xd3\n"
    "1 0 obj\n<< /Type /Catalog >>\nendobj\n"
    "2 0 obj\n<< /Length 10 >>\nstream\nendobj xyz\nendstream\nendobj\n"
    "xref\n0 3\n0000000000 65535 f \n"
    "trailer\n<< /Size 3 /Root 1 0 R /Info << /X 1 >> >>\n"
    "startxref\n123\n"
    "%%EOF\n"
    "1 0 obj\n<< /Type /Catalog /Changed true >>\nendobj\n"
    "xref\n0 1\n"
    "trailer\n<< /Size 3 /Prev 123 >>\n"
    "startxref\n456\n"
    "%%EOF\n";

std::vector<std::string> paths_of(walk_pdf::Content const& content) {
  std::vector<std::string> paths;
  for (auto const& [path, child] : content) {
    paths.push_back(path.to_string());
  }
  return paths;
}

} // namespace

TEST(Document, RootChildrenAreRevisions) {
  Document const document{two_revisions};
  EXPECT_EQ(paths_of(document.root()), (std::vector<std::string>{"/rev[0]", "/rev[1]"}));
}

TEST(Document, RevisionChildren) {
  Document const document{two_revisions};
  auto const first = document.find(*Path::parse("/rev[0]"));
  ASSERT_TRUE(first);
  EXPECT_EQ(paths_of(*first),
            (std::vector<std::string>{"/rev[0]/header", "/rev[0]/comment[0]", "/rev[0]/obj[1]",
                                      "/rev[0]/obj[2]", "/rev[0]/xref[0]", "/rev[0]/trailer[0]",
                                      "/rev[0]/startxref[0]", "/rev[0]/eof[0]"}));
  auto const second = document.find(*Path::parse("/rev[1]"));
  ASSERT_TRUE(second);
  EXPECT_EQ(paths_of(*second),
            (std::vector<std::string>{"/rev[1]/obj[1]", "/rev[1]/xref[0]", "/rev[1]/trailer[0]",
                                      "/rev[1]/startxref[0]", "/rev[1]/eof[0]"}));
}

TEST(Document, SameObjectNumberInEachRevision) {
  Document const document{two_revisions};
  auto const original = document.find(*Path::parse("/rev[0]/obj[1]"));
  auto const updated = document.find(*Path::parse("/rev[1]/obj[1]"));
  ASSERT_TRUE(original);
  ASSERT_TRUE(updated);
  EXPECT_EQ(original->bytes(), "1 0 obj\n<< /Type /Catalog >>\nendobj");
  EXPECT_EQ(updated->bytes(), "1 0 obj\n<< /Type /Catalog /Changed true >>\nendobj");
}

TEST(Document, StreamDataMayContainEndobj) {
  Document const document{two_revisions};
  auto const object = document.find(*Path::parse("/rev[0]/obj[2]"));
  ASSERT_TRUE(object);
  EXPECT_TRUE(object->bytes().ends_with("endstream\nendobj"));
}

TEST(Document, TrailerWithNestedDictionary) {
  Document const document{two_revisions};
  auto const trailer = document.find(*Path::parse("/rev[0]/trailer[0]"));
  ASSERT_TRUE(trailer);
  EXPECT_EQ(trailer->kind(), Kind::trailer);
  EXPECT_EQ(trailer->bytes(), "trailer\n<< /Size 3 /Root 1 0 R /Info << /X 1 >> >>");
}

TEST(Document, FoundContentKnowsItsPathAndOffset) {
  Document const document{two_revisions};
  auto const path = *Path::parse("/rev[0]/header");
  auto const header = document.find(path);
  ASSERT_TRUE(header);
  EXPECT_EQ(header->path(), path);
  EXPECT_EQ(header->offset(), 0U);
  EXPECT_EQ(header->bytes(), "%PDF-1.7");
}

TEST(Document, MissingPath) {
  Document const document{two_revisions};
  EXPECT_FALSE(document.find(*Path::parse("/rev[2]")));
  EXPECT_FALSE(document.find(*Path::parse("/rev[0]/obj[9]")));
  EXPECT_FALSE(document.find(*Path::parse("/rev[1]/header")));
  EXPECT_FALSE(document.find(*Path::parse("/rev[0]/obj[1]/obj[1]")));
}

TEST(Document, UnknownBytesAreGathered) {
  Document const document{"%PDF-1.4\nnot pdf\nstill not\n1 0 obj\nnull\nendobj\n%%EOF\n"};
  auto const unknown = document.find(*Path::parse("/rev[0]/unknown[0]"));
  ASSERT_TRUE(unknown);
  EXPECT_EQ(unknown->bytes(), "not pdf\nstill not\n");
  EXPECT_TRUE(document.find(*Path::parse("/rev[0]/obj[1]")));
}

TEST(Document, ObjectPathHasNoGeneration) {
  Document const document{"%PDF-1.4\n7 3 obj\nnull\nendobj\n%%EOF\n"};
  auto const object = document.find(*Path::parse("/rev[0]/obj[7]"));
  ASSERT_TRUE(object);
  EXPECT_EQ(object->bytes(), "7 3 obj\nnull\nendobj");
}

TEST(Document, DuplicateObjectNumberInARevisionFindsTheFirst) {
  // Only a damaged file defines an object number twice in one revision
  Document const document{"%PDF-1.4\n1 0 obj\n(a)\nendobj\n1 0 obj\n(b)\nendobj\n%%EOF\n"};
  auto const object = document.find(*Path::parse("/rev[0]/obj[1]"));
  ASSERT_TRUE(object);
  EXPECT_EQ(object->bytes(), "1 0 obj\n(a)\nendobj");
}

TEST(Document, ChildrenOfAPath) {
  Document const document{two_revisions};
  std::vector<std::string> paths;
  for (auto const& child : document.children(*Path::parse("/rev[1]"))) {
    paths.push_back(child.path.to_string());
  }
  EXPECT_EQ(paths, paths_of(*document.find(*Path::parse("/rev[1]"))));
  EXPECT_EQ(paths.front(), "/rev[1]/obj[1]");
}

TEST(Document, ChildrenOfAMissingPathAreNone) {
  Document const document{two_revisions};
  auto const children = document.children(*Path::parse("/rev[7]"));
  EXPECT_TRUE(children.begin() == children.end());
}

TEST(Document, ChildrenOfALeafAreNone) {
  Document const document{two_revisions};
  auto const children = document.children(*Path::parse("/rev[0]/eof[0]"));
  EXPECT_TRUE(children.begin() == children.end());
}

TEST(Document, FindGivesTheSameBeforeAndAfterItsParentIsScanned) {
  Document const document{two_revisions};
  auto const path = *Path::parse("/rev[1]/trailer[0]");
  auto const first = document.find(path); // finds it by scanning
  for ([[maybe_unused]] auto const& child : document.children(path.parent())) {
  }
  auto const again = document.find(path); // finds it in the index
  ASSERT_TRUE(first);
  ASSERT_TRUE(again);
  EXPECT_EQ(first->bytes(), again->bytes());
  EXPECT_EQ(first->offset(), again->offset());
  EXPECT_EQ(again->kind(), Kind::trailer);
  EXPECT_EQ(again->path(), path);
}

TEST(Document, MissingSiblingOfAFoundElement) {
  Document const document{two_revisions};
  ASSERT_TRUE(document.find(*Path::parse("/rev[0]/obj[1]")));
  EXPECT_FALSE(document.find(*Path::parse("/rev[0]/obj[3]")));
  EXPECT_FALSE(document.find(*Path::parse("/rev[0]/obj[3]")));
}

TEST(Document, ClientDepthFirstWalkOnPaths) {
  // How a client walks the file: on paths, with its own depth counter
  Document const document{two_revisions};
  std::vector<std::string> visited;
  auto walk = [&](auto const& self, Path const& path, int depth) -> void {
    visited.push_back(path.to_string());
    if (depth == 0) {
      return;
    }
    for (auto const& child : document.children(path)) {
      self(self, child.path, depth - 1);
    }
  };
  walk(walk, Path{}, 1);
  EXPECT_EQ(visited, (std::vector<std::string>{"/", "/rev[0]", "/rev[1]"}));
  visited.clear();
  walk(walk, *Path::parse("/rev[1]"), 1);
  EXPECT_EQ(visited.size(), 6U);
  EXPECT_EQ(visited.back(), "/rev[1]/eof[0]");
}

TEST(Document, ObjectChildrenAreItsEntriesThenItsStream) {
  Document const document{two_revisions};
  EXPECT_EQ(paths_of(*document.find(*Path::parse("/rev[0]/obj[2]"))),
            (std::vector<std::string>{"/rev[0]/obj[2]/Length", "/rev[0]/obj[2]/stream"}));
  auto const data = document.find(*Path::parse("/rev[0]/obj[2]/stream"));
  ASSERT_TRUE(data);
  EXPECT_EQ(data->kind(), Kind::stream);
  EXPECT_EQ(data->bytes(), "endobj xyz");
}

TEST(Document, TrailerChildrenAreItsEntries) {
  Document const document{two_revisions};
  EXPECT_EQ(paths_of(*document.find(*Path::parse("/rev[0]/trailer[0]"))),
            (std::vector<std::string>{"/rev[0]/trailer[0]/Size", "/rev[0]/trailer[0]/Root",
                                      "/rev[0]/trailer[0]/Info"}));
  auto const root = document.find(*Path::parse("/rev[0]/trailer[0]/Root"));
  ASSERT_TRUE(root);
  EXPECT_EQ(root->kind(), Kind::reference);
  EXPECT_EQ(root->bytes(), "1 0 R");
}

TEST(Document, NestedDictionariesAndArrays) {
  Document const document{"%PDF-1.4\n5 0 obj\n<< /Type /Page /MediaBox [0 0 595 842]\n"
                          "/Resources << /Font << /F8 9 0 R >> >> >>\nendobj\n%%EOF\n"};
  auto const font = document.find(*Path::parse("/rev[0]/obj[5]/Resources/Font/F8"));
  ASSERT_TRUE(font);
  EXPECT_EQ(font->bytes(), "9 0 R");
  auto const width = document.find(*Path::parse("/rev[0]/obj[5]/MediaBox/[2]"));
  ASSERT_TRUE(width);
  EXPECT_EQ(width->kind(), Kind::integer);
  EXPECT_EQ(width->bytes(), "595");
  EXPECT_EQ(document.find(*Path::parse("/rev[0]/obj[5]/Type"))->kind(), Kind::name);
  EXPECT_FALSE(document.find(*Path::parse("/rev[0]/obj[5]/MediaBox/[4]")));
  EXPECT_FALSE(document.find(*Path::parse("/rev[0]/obj[5]/Missing")));
}

TEST(Document, ScalarObjectHasNoChildren) {
  Document const document{"%PDF-1.4\n1 0 obj\n42\nendobj\n%%EOF\n"};
  EXPECT_TRUE(paths_of(*document.find(*Path::parse("/rev[0]/obj[1]"))).empty());
}

TEST(Document, WalkBeyondTheIndexCapacity) {
  // More elements than the index holds: lookups during a walk still succeed
  std::string text = "%PDF-1.4\n1 0 obj\n[";
  for (int i = 0; i < 70000; ++i) {
    text += std::to_string(i) + " ";
  }
  text += "]\nendobj\n%%EOF\n";
  Document const document{text};
  auto const array = *Path::parse("/rev[0]/obj[1]");
  std::size_t found = 0;
  for (auto const& child : document.children(array)) {
    auto const content = document.find(child.path);
    ASSERT_TRUE(content) << child.path.to_string();
    EXPECT_EQ(content->bytes(), std::to_string(found));
    ++found;
  }
  EXPECT_EQ(found, 70000U);
  EXPECT_EQ(document.find(*Path::parse("/rev[0]/obj[1]/[3]"))->bytes(), "3"); // after the index started over
}

TEST(Document, StreamIsDecoded) {
  std::string const page = "BT /F1 12 Tf (Hello) Tj ET";
  auto compressed = page;
  {
    auto size = compressBound(static_cast<uLong>(page.size()));
    compressed.assign(size, '\0');
    compress(reinterpret_cast<Bytef*>(compressed.data()), &size,
             reinterpret_cast<Bytef const*>(page.data()), static_cast<uLong>(page.size()));
    compressed.resize(size);
  }
  Document const document{"%PDF-1.4\n1 0 obj\n<< /Length " + std::to_string(compressed.size()) +
                          " /Filter /FlateDecode >>\nstream\n" + compressed +
                          "\nendstream\nendobj\n%%EOF\n"};
  EXPECT_EQ(paths_of(*document.find(*Path::parse("/rev[0]/obj[1]/stream"))),
            (std::vector<std::string>{"/rev[0]/obj[1]/stream/decoded"}));
  auto const decoded = document.find(*Path::parse("/rev[0]/obj[1]/stream/decoded"));
  ASSERT_TRUE(decoded);
  EXPECT_EQ(decoded->kind(), Kind::decoded);
  EXPECT_EQ(decoded->bytes(), page);
  EXPECT_FALSE(decoded->in_file());
  EXPECT_EQ(decoded->offset(), 0U);
  EXPECT_EQ(paths_of(*decoded), (std::vector<std::string>{"/rev[0]/obj[1]/stream/decoded/BT[0]"}));
}

TEST(Document, ContentStreamTextBlocksOperationsAndOperands) {
  std::string const page = "q 1 0 0 1 0 0 cm\nBT /F1 12 Tf 72 712 Td (Hello) Tj [(W) 120 (orld)] TJ ET\n"
                           "0 0 m 10 10 l S\nBT /F2 9 Tf <0001> Tj ET Q";
  Document const document{"%PDF-1.4\n1 0 obj\n<< /Length " + std::to_string(page.size()) +
                          " >>\nstream\n" + page + "\nendstream\nendobj\n%%EOF\n"};
  auto const decoded = *Path::parse("/rev[0]/obj[1]/stream/decoded");
  EXPECT_EQ(paths_of(*document.find(decoded)),
            (std::vector<std::string>{"/rev[0]/obj[1]/stream/decoded/BT[0]", "/rev[0]/obj[1]/stream/decoded/BT[1]"}));
  auto const block = decoded.child(walk_pdf::Segment::ordinal(Kind::text_block, 0));
  EXPECT_EQ(document.find(block)->bytes(), "BT /F1 12 Tf 72 712 Td (Hello) Tj [(W) 120 (orld)] TJ ET");
  std::vector<std::string> operations;
  for (auto const& child : document.children(block)) {
    operations.emplace_back(child.content.bytes());
  }
  EXPECT_EQ(operations, (std::vector<std::string>{"/F1 12 Tf", "72 712 Td", "(Hello) Tj", "[(W) 120 (orld)] TJ"}));
  auto const tj = document.find(*Path::parse("/rev[0]/obj[1]/stream/decoded/BT[0]/op[3]/[0]/[2]"));
  ASSERT_TRUE(tj);
  EXPECT_EQ(tj->kind(), Kind::string);
  EXPECT_EQ(tj->bytes(), "(orld)");
  EXPECT_EQ(document.find(*Path::parse("/rev[0]/obj[1]/stream/decoded/BT[1]/op[1]/[0]"))->bytes(), "<0001>");
  EXPECT_FALSE(document.find(*Path::parse("/rev[0]/obj[1]/stream/decoded/BT[0]/op[0]/[2]"))); // Tf itself
}

TEST(Document, OtherStreamsAreNotContentStreams) {
  // A ToUnicode CMap and a font program decode, but have no text blocks
  std::string const cmap = "/CIDInit /ProcSet findresource begin begincmap BT 1 beginbfchar <01> <0041> endbfchar ET";
  Document const document{"%PDF-1.4\n1 0 obj\n<< /Length " + std::to_string(cmap.size()) + " >>\nstream\n" +
                          cmap + "\nendstream\nendobj\n2 0 obj\n<< /Length1 9 /Length 9 >>\nstream\nBT x Tj ET\n"
                          "endstream\nendobj\n3 0 obj\n<< /Type /XObject /Subtype /Image /Length 9 >>\nstream\n"
                          "BT x Tj ET\nendstream\nendobj\n%%EOF\n"};
  for (auto const* path : {"/rev[0]/obj[1]/stream/decoded", "/rev[0]/obj[2]/stream/decoded",
                           "/rev[0]/obj[3]/stream/decoded"}) {
    ASSERT_TRUE(document.find(*Path::parse(path))) << path;
    EXPECT_TRUE(paths_of(*document.find(*Path::parse(path))).empty()) << path;
  }
}

TEST(Document, InlineImageIsOneOperation) {
  std::string const page = "BT (a) Tj ET BI /W 2 /H 1 /BPC 8 /CS /G ID \x01ET\xFF\nEI BT (b) Tj ET";
  Document const document{"%PDF-1.4\n1 0 obj\n<< /Length " + std::to_string(page.size()) + " >>\nstream\n" +
                          page + "\nendstream\nendobj\n%%EOF\n"};
  EXPECT_EQ(paths_of(*document.find(*Path::parse("/rev[0]/obj[1]/stream/decoded"))).size(), 2U);
  EXPECT_EQ(document.find(*Path::parse("/rev[0]/obj[1]/stream/decoded/BT[1]"))->bytes(), "BT (b) Tj ET");
}

TEST(Document, UndecodableStreamHasNoDecodedChild) {
  Document const document{"%PDF-1.4\n1 0 obj\n<< /Length 4 /Filter /DCTDecode >>\nstream\njpeg\n"
                          "endstream\nendobj\n%%EOF\n"};
  EXPECT_TRUE(paths_of(*document.find(*Path::parse("/rev[0]/obj[1]/stream"))).empty());
  EXPECT_FALSE(document.find(*Path::parse("/rev[0]/obj[1]/stream/decoded")));
}

TEST(Document, StreamWithoutFiltersIsDecodedAsItIs) {
  Document const document{"%PDF-1.4\n1 0 obj\n<< /Length 13 >>\nstream\nBT (Hi) Tj ET\n"
                          "endstream\nendobj\n%%EOF\n"};
  EXPECT_EQ(document.find(*Path::parse("/rev[0]/obj[1]/stream/decoded"))->bytes(), "BT (Hi) Tj ET");
}

TEST(Document, EmptyFileHasNoRevisions) {
  Document const document{""};
  EXPECT_TRUE(paths_of(document.root()).empty());
  EXPECT_TRUE(document.find(Path{}));
}
