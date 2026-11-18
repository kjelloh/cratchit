#include "core/date.hpp"

#include <gtest/gtest.h>

using namespace std::chrono;
using cratchit::format_iso_date;
using cratchit::next_day;
using cratchit::parse_compact_date;
using cratchit::parse_iso_date;

TEST(Date, CompactAndIso) {
  EXPECT_EQ(parse_compact_date("20260501"), year_month_day(year{2026}, May, day{1}));
  EXPECT_EQ(parse_iso_date("2026-05-01"), year_month_day(year{2026}, May, day{1}));
  EXPECT_EQ(format_iso_date(year_month_day(year{2026}, May, day{1})), "2026-05-01");
}

TEST(Date, InvalidDatesAreRejected) {
  EXPECT_FALSE(parse_compact_date("20260230"));
  EXPECT_FALSE(parse_compact_date("2026051"));
  EXPECT_FALSE(parse_compact_date("2026-5-1"));
  EXPECT_FALSE(parse_compact_date(""));
  EXPECT_FALSE(parse_iso_date("20260501"));
  EXPECT_FALSE(parse_iso_date("2026-13-01"));
}

TEST(Date, NextDayCrossesMonthAndYear) {
  EXPECT_EQ(next_day(year_month_day(year{2026}, April, day{30})), year_month_day(year{2026}, May, day{1}));
  EXPECT_EQ(next_day(year_month_day(year{2025}, December, day{31})), year_month_day(year{2026}, January, day{1}));
}
