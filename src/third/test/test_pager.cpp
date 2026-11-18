#include "pyroceph/tui/pager.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using pyroceph::tui::Pager;
using pyroceph::tui::View;

namespace {

std::vector<std::string> numbered(int count) {
  std::vector<std::string> lines;
  for (int i = 1; i <= count; ++i) lines.push_back(std::to_string(i));
  return lines;
}

} // namespace

TEST(Pager, ShowsNothingForNoLines) {
  Pager pager(3);
  pager.set({});
  EXPECT_TRUE(pager.view().empty());
  EXPECT_FALSE(pager.can_back());
  EXPECT_FALSE(pager.can_forward());
}

TEST(Pager, AShortListFitsWithoutMore) {
  Pager pager(3);
  pager.set(numbered(3));
  EXPECT_EQ(pager.view(), (View{"1", "2", "3"}));
  EXPECT_FALSE(pager.forward());
}

TEST(Pager, TurnsAWholePageAndMarksWhatIsBeforeAndAfter) {
  Pager pager(3);
  pager.set(numbered(7));
  EXPECT_EQ(pager.view(), (View{"1", "2", "3", "  …"}));
  EXPECT_FALSE(pager.back());
  ASSERT_TRUE(pager.forward());
  EXPECT_EQ(pager.view(), (View{"  …", "4", "5", "6", "  …"}));
  EXPECT_EQ(pager.first(), 3u);
  EXPECT_EQ(pager.shown(), 3u);
  ASSERT_TRUE(pager.forward());
  EXPECT_EQ(pager.view(), (View{"  …", "7"}));
  EXPECT_FALSE(pager.forward());
  ASSERT_TRUE(pager.back());
  ASSERT_TRUE(pager.back());
  EXPECT_EQ(pager.view(), (View{"1", "2", "3", "  …"}));
}

TEST(Pager, StaysOnAWholePageWhenTheListGetsShorter) {
  Pager pager(3);
  pager.set(numbered(7));
  pager.forward();
  pager.forward(); // on 7
  pager.set(numbered(5));
  EXPECT_EQ(pager.view(), (View{"  …", "4", "5"}));
}
