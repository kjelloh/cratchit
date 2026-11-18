#include "core/skv/vat_return/account_boxes.hpp"
#include "core/skv/vat_return/box_tags.hpp"
#include "core/skv/vat_return/eskd.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace cratchit;
using namespace cratchit::skv::vat_return;
using namespace std::chrono;

TEST(Eskd, BuiltInTagsAreInFileOrder) {
  auto const tags = box_tags(box_tags_text());
  ASSERT_TRUE(tags) << tags.error();
  ASSERT_EQ(tags->size(), 29u);
  EXPECT_EQ(tags->front(), (BoxTag{"05", "ForsMomsEjAnnan"}));
  EXPECT_EQ((*tags)[4], (BoxTag{"10", "MomsUtgHog"}));
  EXPECT_EQ((*tags)[23], (BoxTag{"50", "MomsUlagImport"}));
  EXPECT_EQ((*tags)[27], (BoxTag{"48", "MomsIngAvdr"}));
  EXPECT_EQ(tags->back(), (BoxTag{"49", "MomsBetala"}));
}

TEST(Eskd, EveryBoxAnAccountReportsInHasATag) {
  auto const tags = box_tags(box_tags_text());
  auto const boxes = account_boxes(account_boxes_text());
  ASSERT_TRUE(tags) << tags.error();
  ASSERT_TRUE(boxes) << boxes.error();
  for (auto const& [account, box] : *boxes) {
    EXPECT_NE(std::ranges::find(*tags, box, &BoxTag::box), tags->end()) << account << " in box " << box;
  }
}

TEST(Eskd, TagsSkipHeadingBomCrAndBlankLines) {
  auto const tags = box_tags("\xEF\xBB\xBF" "box;tag\r\n05;ForsMomsEjAnnan\r\n\r\n49;MomsBetala\r\n");
  ASSERT_TRUE(tags) << tags.error();
  EXPECT_EQ(*tags, (std::vector<BoxTag>{{"05", "ForsMomsEjAnnan"}, {"49", "MomsBetala"}}));
}

TEST(Eskd, TagsErrorsNameTheLine) {
  EXPECT_EQ(box_tags("box;tag\n5;MomsUtgHog\n").error(), "Line 2: the box is not two digits: 5;MomsUtgHog");
  EXPECT_EQ(box_tags("10;Moms Utg\n").error(), "Line 1: the tag is not ASCII letters and digits: 10;Moms Utg");
  EXPECT_EQ(box_tags("10\n").error(), "Line 1: the tag is not ASCII letters and digits: 10");
  EXPECT_EQ(box_tags("10;MomsUtgHog\n10;MomsUtgMedel\n").error(), "Line 2: box 10 is listed before");
  EXPECT_EQ(box_tags("10;MomsUtgHog\n11;MomsUtgHog\n").error(), "Line 2: tag MomsUtgHog is listed before");
}

TEST(Eskd, DocumentAsTheFiledReturnOfQ1_2026) {
  // moms_202603.eskd: EU purchase 124, its output VAT 31, input VAT 2051, 2020 to get back
  Document const form{*quarter_of(2026y, 1), {{"20", 12400, {}}, {"30", 3100, {}}, {"48", 205100, {}}}};
  EXPECT_EQ(eskd::document_of(form, "556782-8172"),
            (eskd::Document{"556782-8172",
                            "202603",
                            {{"InkopVaruAnnatEg", 12400},
                             {"MomsInkopUtgHog", 3100},
                             {"MomsIngAvdr", 205100},
                             {"MomsBetala", -202000}}}));
}

TEST(Eskd, DocumentOfTheBooksTakesTheFormFigures) {
  books::Books books;
  books.years.push_back({.start = 2026y / January / 1, .end = 2026y / December / 31});
  books.years[0].vouchers.push_back({.series = "B",
                                     .number = "1",
                                     .event = {"Sale", 125000, 2026y / August / 10},
                                     .registered = std::nullopt,
                                     .signature = {},
                                     .postings = {{.account = "1930", .amount = 125000},
                                                  {.account = "3001", .amount = -100000},
                                                  {.account = "2611", .amount = -25000}}});
  auto const document = eskd::document_of(document_of(books, *quarter_of(2026y, 3)), "556000-0175");
  EXPECT_EQ(document.period, "202609");
  EXPECT_EQ(document.fields,
            (std::vector<eskd::Field>{{"ForsMomsEjAnnan", 100000}, {"MomsUtgHog", 25000}, {"MomsBetala", 25000}}));
}

TEST(Eskd, DocumentWithoutBoxesHasMomsBetalaOnly) {
  auto const document = eskd::document_of(Document{*quarter_of(2026y, 4), {}}, "556000-0175");
  EXPECT_EQ(document.period, "202612");
  EXPECT_EQ(document.fields, (std::vector<eskd::Field>{{"MomsBetala", 0}}));
}

TEST(Eskd, XmlOfTheFiledReturnOfQ1_2026) {
  eskd::Document const document{"556782-8172",
                                "202603",
                                {{"InkopVaruAnnatEg", 12400},
                                 {"MomsInkopUtgHog", 3100},
                                 {"MomsIngAvdr", 205100},
                                 {"MomsBetala", -202000}}};
  EXPECT_EQ(eskd::xml_of(document), "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n"
                                    "<eSKDUpload Version=\"6.0\">\n"
                                    "  <OrgNr>556782-8172</OrgNr>\n"
                                    "  <Moms>\n"
                                    "    <Period>202603</Period>\n"
                                    "    <InkopVaruAnnatEg>124</InkopVaruAnnatEg>\n"
                                    "    <MomsInkopUtgHog>31</MomsInkopUtgHog>\n"
                                    "    <MomsIngAvdr>2051</MomsIngAvdr>\n"
                                    "    <MomsBetala>-2020</MomsBetala>\n"
                                    "  </Moms>\n"
                                    "</eSKDUpload>\n");
}

TEST(Eskd, FileNames) {
  EXPECT_EQ(eskd::file_name("202603"), "moms_202603.eskd");
  EXPECT_EQ(eskd::file_name("202603", 2), "moms_202603_2.eskd");
}

TEST(Eskd, WriteFileNeverOverwrites) {
  auto const folder = std::filesystem::temp_directory_path() / "cratchit_eskd_write";
  std::filesystem::remove_all(folder);
  std::filesystem::create_directories(folder);
  eskd::Document const first{"556000-0175", "202603", {{"MomsBetala", 10000}}};
  eskd::Document const second{"556000-0175", "202603", {{"MomsBetala", 20000}}};
  auto const written = eskd::write_file(folder, first);
  ASSERT_TRUE(written) << written.error();
  EXPECT_EQ(*written, folder / "moms_202603.eskd");
  auto const again = eskd::write_file(folder, second);
  ASSERT_TRUE(again) << again.error();
  EXPECT_EQ(*again, folder / "moms_202603_2.eskd");
  auto const text = [](std::filesystem::path const& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string{std::istreambuf_iterator<char>(in), {}};
  };
  EXPECT_EQ(text(*written), eskd::xml_of(first));
  EXPECT_EQ(text(*again), eskd::xml_of(second));
  EXPECT_EQ(eskd::free_file(folder, "202603"), folder / "moms_202603_3.eskd");
  std::filesystem::remove_all(folder);
}
