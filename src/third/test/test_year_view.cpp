#include "ui/year_view.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace std::chrono;
using cratchit::books::Draft;
using cratchit::books::FiscalYear;
using cratchit::books::Posting;
using cratchit::books::Voucher;
using cratchit::ui::year_view;
using Rows = std::vector<std::string>;

namespace {

cratchit::Date date(int y, unsigned m, unsigned d) {
  return year_month_day(year{y}, month{m}, day{d});
}

Posting row(std::string account, cratchit::Amount amount) {
  return {Posting::Kind::normal, std::move(account), {}, amount, {}, "", "", ""};
}

Voucher voucher(std::string number, cratchit::Date when, std::string heading, cratchit::Amount amount) {
  return {"A", std::move(number), {std::move(heading), amount, when}, when, "", {row("1930", amount), row("3010", -amount)}};
}

FiscalYear year_with(std::vector<Voucher> vouchers) {
  FiscalYear year{date(2026, 5, 1), date(2027, 4, 30), {}, std::move(vouchers)};
  return year;
}

} // namespace

TEST(YearView, AYearNotInTheBooksHasNothingBooked) {
  auto const view = year_view(nullptr);
  EXPECT_EQ(view.summary, "Hela året: inget bokfört");
  EXPECT_TRUE(view.rows.empty());
}

TEST(YearView, TheSummaryCountsVouchersAndDrafts) {
  auto year = year_with({voucher("1", date(2026, 6, 1), "Sale", 10000)});
  EXPECT_EQ(year_view(&year).summary, "Hela året: 1 verifikation");
  year.vouchers.push_back(voucher("2", date(2026, 6, 2), "Sale", 10000));
  EXPECT_EQ(year_view(&year).summary, "Hela året: 2 verifikationer");
  year.place_draft({{"Telia", 111800, date(2026, 10, 20)}, date(2026, 10, 23)});
  EXPECT_EQ(year_view(&year).summary, "Hela året: 2 verifikationer, 1 utkast");
  year.vouchers.clear();
  EXPECT_EQ(year_view(&year).summary, "Hela året: inget bokfört, 1 utkast");
}

TEST(YearView, RowsAreNewestFirst) {
  // In the books out of date order, as an SIE file may have them
  auto const year = year_with({voucher("1", date(2026, 6, 1), "Första", 10000),
                               voucher("3", date(2026, 9, 3), "Kontorsmaterial", 41250),
                               voucher("2", date(2026, 6, 1), "Andra", 20000)});
  EXPECT_EQ(year_view(&year).rows, (Rows{"  A3            2026-09-03        412.50  Kontorsmaterial",
                                         "  A2            2026-06-01        200.00  Andra",
                                         "  A1            2026-06-01        100.00  Första"}));
}

TEST(YearView, DraftsShowTheirStageAndComeBeforeVouchersOfTheSameDate) {
  auto year = year_with({voucher("1", date(2026, 10, 20), "Sale", 10000)});
  year.place_draft({{"Telia oktober", 111800, date(2026, 10, 20)}, date(2026, 10, 23)});
  year.place_draft({{"", 0, date(2026, 10, 21)}, date(2026, 10, 23), {row("1930", -111800)}});
  year.place_draft({{"Loopia", 130875, date(2026, 9, 28)}, date(2026, 10, 23),
                    {row("6540", 130875), row("1930", -130875)}});
  EXPECT_EQ(year_view(&year).rows, (Rows{"  kontohändelse 2026-10-21      -1118.00  ",
                                         "  händelse      2026-10-20       1118.00  Telia oktober",
                                         "  A1            2026-10-20        100.00  Sale",
                                         "  klart         2026-09-28       1308.75  Loopia"}));
}

TEST(YearView, AtMostMaxRowsAreShownThenHowManyOlder) {
  std::vector<Voucher> vouchers;
  for (unsigned day = 1; day <= 12; ++day) {
    vouchers.push_back(voucher(std::to_string(day), date(2026, 6, day), "Sale", 100));
  }
  auto const year = year_with(std::move(vouchers));
  auto const rows = year_view(&year).rows;
  ASSERT_EQ(rows.size(), 11u);
  EXPECT_TRUE(rows[0].starts_with("  A12 ")) << rows[0];
  EXPECT_TRUE(rows[9].starts_with("  A3 ")) << rows[9];
  EXPECT_EQ(rows[10], "  … 2 äldre");
  EXPECT_EQ(year_view(&year, 12).rows.size(), 12u); // all shown: no row about older ones
}

TEST(DraftRows, NewestFirstWithIdFromNewestAndTextOrPosting) {
  auto year = year_with({voucher("1", date(2026, 10, 20), "Sale", 10000)});
  year.place_draft({{"Telia oktober", 111800, date(2026, 10, 20)}, date(2026, 10, 23)});
  auto fee = row("1920", -185);
  fee.text = "AVGIFTER NORDEA";
  year.place_draft({{"", 0, date(2026, 10, 21)}, date(2026, 10, 23), {fee}});
  year.place_draft({{"", 0, date(2026, 9, 3)}, date(2026, 10, 23), {row("1630", 100)}});
  EXPECT_EQ(cratchit::ui::draft_rows(&year),
            (Rows{"  #1            2026-10-21         -1.85  1920  AVGIFTER NORDEA (kontohändelse)",
                  "  #2            2026-10-20       1118.00  Telia oktober (händelse)",
                  "  #3            2026-09-03          1.00  1630 (kontohändelse)"}));
  EXPECT_TRUE(cratchit::ui::draft_rows(nullptr).empty());
  // The oldest has the highest id: booking it keeps the others' ids
  EXPECT_EQ(cratchit::ui::draft_id(0, 3), "#3");
  EXPECT_EQ(cratchit::ui::draft_id(2, 3), "#1");
  EXPECT_EQ(cratchit::ui::draft_id(0, 2), "#2");
  EXPECT_EQ(cratchit::ui::draft_index(1, 3), 2u);
  EXPECT_EQ(cratchit::ui::draft_index(3, 3), 0u);
  EXPECT_FALSE(cratchit::ui::draft_index(0, 3));
  EXPECT_FALSE(cratchit::ui::draft_index(4, 3));
}

TEST(DraftRows, OnlyAPostingAloneShowsItsSign) {
  auto year = year_with({});
  // One posting out of the account: its sign shows
  year.place_draft({{"", 0, date(2026, 7, 21)}, date(2026, 11, 17), {row("1630", -229200)}});
  // Two postings and no amount of its own: what they move, without sign
  year.place_draft({{"", 0, date(2026, 7, 22)}, date(2026, 11, 17), {row("1930", -500), row("6570", 300)}});
  auto const rows = cratchit::ui::draft_rows(&year);
  ASSERT_EQ(rows.size(), 2u);
  EXPECT_TRUE(rows[1].starts_with("  #2            2026-07-21      -2292.00  1630")) << rows[1];
  EXPECT_TRUE(rows[0].starts_with("  #1            2026-07-22          5.00  1930")) << rows[0];
}

TEST(DraftRows, TheSameEventShowsTheRawTextsOfItsParts) {
  auto year = year_with({});
  // A transfer: two account events, no heading, no accounts in the text
  auto out = row("1630", -229200);
  out.text = "Utbetalning";
  auto in = row("1920", 229200);
  in.text = "Skatt | 5050-1030 SK5567828172";
  year.place_draft({{"", 0, date(2026, 7, 21)}, date(2026, 11, 18), {out, in}});
  // An event and its payment: the heading first; a part without text adds nothing
  auto paid = row("1920", -20468);
  paid.text = "Kortköp 260910 ANTHROPIC* CLAUDE SU";
  auto silent = row("1930", 20468);
  year.place_draft({{"Anthropic Claude", 20468, date(2026, 9, 10)}, date(2026, 11, 18), {paid}});
  auto const rows = cratchit::ui::draft_rows(&year);
  ASSERT_EQ(rows.size(), 2u);
  EXPECT_EQ(rows[1], "  #2            2026-07-21       2292.00  Utbetalning : Skatt | 5050-1030 SK5567.. (samma)");
  EXPECT_EQ(rows[0], "  #1            2026-09-10        204.68  Anthropic Claude : Kortköp 260910 ANTH.. (samma)");
  // Other stages as before: the heading alone
  Draft unbalanced{{"Anthropic Claude", 20468, date(2026, 9, 10)}, date(2026, 11, 18), {paid, silent, row("6540", 100)}};
  EXPECT_EQ(cratchit::ui::draft_text(unbalanced), "Anthropic Claude (obalanserat)");
}

TEST(DraftRows, TheSameEventTextIsAtMost40ShortPartsWhole) {
  auto year = year_with({});
  auto out = row("1920", -500000);
  out.text = "Till sparkonto";
  auto in = row("1940", 500000);
  in.text = "Från PlusGiro";
  // Three parts: the short heading whole, the two others share what is left (12 each)
  year.place_draft({{"Överföring", 0, date(2026, 9, 3)}, date(2026, 11, 18), {out, in}});
  // Short enough: nothing cut
  auto fee = row("1930", -1000);
  fee.text = "AVGIFTER";
  year.place_draft({{"Bankavgift", 1000, date(2026, 9, 4)}, date(2026, 11, 18), {fee}});
  auto const rows = cratchit::ui::draft_rows(&year);
  ASSERT_EQ(rows.size(), 2u);
  EXPECT_TRUE(rows[1].ends_with("  Överföring : Till spark.. : Från PlusG.. (samma)")) << rows[1];
  EXPECT_TRUE(rows[0].ends_with("  Bankavgift : AVGIFTER (samma)")) << rows[0];
}
