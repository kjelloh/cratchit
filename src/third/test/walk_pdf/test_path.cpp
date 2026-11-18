#include "path.hpp"

#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>

using walk_pdf::Kind;
using walk_pdf::Path;
using walk_pdf::Segment;

TEST(Path, RootIsSlash) {
  EXPECT_EQ(Path{}.to_string(), "/");
  EXPECT_TRUE(Path{}.is_root());
}

TEST(Path, ToString) {
  auto const path = Path{}.child(Segment::revision(1)).child(Segment::object(12));
  EXPECT_EQ(path.to_string(), "/rev[1]/obj[12]");
  EXPECT_EQ(Path{}.child(Segment::revision(0)).child(Segment::header()).to_string(),
            "/rev[0]/header");
  EXPECT_EQ(Path{}.child(Segment::ordinal(Kind::xref, 2)).to_string(), "/xref[2]");
}

TEST(Path, ParentAndChild) {
  auto const revision = Path{}.child(Segment::revision(0));
  auto const object = revision.child(Segment::object(5));
  EXPECT_EQ(object.parent(), revision);
  EXPECT_EQ(revision.parent(), Path{});
  EXPECT_EQ(Path{}.parent(), Path{});
}

TEST(Path, RoundTrip) {
  for (auto const* text :
       {"/", "/rev[0]", "/rev[0]/header", "/rev[1]/obj[73867]", "/rev[0]/comment[3]",
        "/rev[2]/xref[0]", "/rev[2]/trailer[0]", "/rev[2]/startxref[0]", "/rev[2]/eof[0]",
        "/rev[0]/unknown[1]", "/rev[0]/obj[18446744073709551615]",
        "/rev[0]/obj[5]/Resources/Font/F8", "/rev[0]/obj[5]/MediaBox/[3]", "/rev[0]/obj[35]/stream",
        "/rev[0]/trailer[0]/Root", "/rev[0]/obj[1]/A#20B", "/rev[0]/obj[1]/#68eader",
        "/rev[0]/obj[1]/#73tream", "/rev[0]/obj[1]/#", "/rev[0]/obj[1]/rev"}) {
    auto const path = Path::parse(text);
    ASSERT_TRUE(path) << text << ": " << path.error();
    EXPECT_EQ(path->to_string(), text);
    EXPECT_EQ(Path::parse(path->to_string()), path);
    EXPECT_EQ(path->to_string().find(' '), std::string::npos) << text;
  }
}

TEST(Path, ObjectIsItsNumber) {
  auto const path = Path::parse("/rev[0]/obj[12]");
  ASSERT_TRUE(path);
  EXPECT_EQ(path->segments().back(), Segment::object(12)); // "12 0 obj" in the file
  EXPECT_EQ(Path{}.child(Segment::object(73867)).to_string(), "/obj[73867]");
}

TEST(Path, ParseRejectsInvalid) {
  for (auto const* text :
       {"", "rev[0]", "/rev[0]/", "//", "/rev[]", "/rev[x]", "/rev[-1]", "/rev[0", "/header[0]",
        "/obj[]", "/obj[5 0]", "/obj[0.5]", "/obj[-1]", "/obj[x]", "/sheet[0]", "/[]", "/[x]",
        "/[1", "/A#2", "/A#ZZ", "/A(B", "/A%B", "/stream[0]"}) {
    EXPECT_FALSE(Path::parse(text)) << text;
  }
}

TEST(Path, KeysIndexesAndStream) {
  auto const path = Path{}
                        .child(Segment::revision(0))
                        .child(Segment::object(5))
                        .child(Segment::entry("Resources"))
                        .child(Segment::item(2))
                        .child(Segment::stream());
  EXPECT_EQ(path.to_string(), "/rev[0]/obj[5]/Resources/[2]/stream");
  EXPECT_EQ(Path::parse(path.to_string()), path);
}

TEST(Path, KeysAreEscapedLikePdfNames) {
  EXPECT_EQ(Segment::entry("A B").to_string(), "A#20B");
  EXPECT_EQ(Segment::entry("a/b[1]").to_string(), "a#2Fb#5B1#5D");
  EXPECT_EQ(Segment::entry("header").to_string(), "#68eader"); // not the header segment
  EXPECT_EQ(Segment::entry("stream").to_string(), "#73tream"); // not the stream segment
  EXPECT_EQ(Segment::entry("").to_string(), "#");
  EXPECT_EQ(Segment::entry("Malm\xf6").to_string(), "Malm#F6");
  EXPECT_EQ(Path::parse("/A#20B")->segments().back(), Segment::entry("A B"));
  EXPECT_EQ(Path::parse("/#68eader")->segments().back(), Segment::entry("header"));
  EXPECT_EQ(Path::parse("/header")->segments().back(), Segment::header());
  EXPECT_EQ(Path::parse("/stream")->segments().back(), Segment::stream());
}

TEST(Path, IsAValueThatOrders) {
  auto const a = *Path::parse("/rev[0]/obj[1]");
  auto const b = *Path::parse("/rev[0]/obj[2]");
  EXPECT_LT(a, b);
  std::map<Path, std::string> notes{{b, "second"}, {a, "first"}};
  EXPECT_EQ(notes.begin()->second, "first");
}
