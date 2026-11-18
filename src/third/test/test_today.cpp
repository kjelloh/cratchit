// Every test runs on the same day, so that what depends on today (e.g. the
// fiscal years listed) does not change with the clock
#include "core/dot_folder.hpp"

#include <gtest/gtest.h>

namespace {

class FixedToday : public ::testing::Environment {
public:
  void SetUp() override { cratchit::set_today("2026-11-09"); }
  void TearDown() override { cratchit::set_today(std::nullopt); }
};

[[maybe_unused]] auto* const fixed_today = ::testing::AddGlobalTestEnvironment(new FixedToday);

} // namespace

TEST(Today, IsTheDaySetForTheTests) { EXPECT_EQ(cratchit::today_iso(), "2026-11-09"); }

TEST(Today, CanBeSetAndUnset) {
  cratchit::set_today("2027-05-02");
  EXPECT_EQ(cratchit::today_iso(), "2027-05-02");
  cratchit::set_today(std::nullopt);
  EXPECT_EQ(cratchit::today_iso().size(), 10u); // the clock's date
  cratchit::set_today("2026-11-09");
}
