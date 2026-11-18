// Rapporter in the fiscal year (ui/reports_menu): the menu, a report a page at
// a time, and Huvudbok's account number
#include "core/sie_project.hpp"
#include "ui/help_topics.hpp"
#include "ui/reports_menu.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using pyroceph::tui::Navigator;
using pyroceph::tui::OptionsList;

namespace {

// A folder of its own for each test, removed afterwards
class ReportsMenuTest : public ::testing::Test {
protected:
  void SetUp() override {
    auto const* info = ::testing::UnitTest::GetInstance()->current_test_info();
    documents = fs::temp_directory_path() / (std::string("cratchit_reports_test_") + info->name());
    fs::remove_all(documents);
    fs::create_directories(documents);
  }
  void TearDown() override { fs::remove_all(documents); }

  // A project from an SIE file of 2025 with an opening balance on 1930 and 2099, and
  // `sales` vouchers A 1, A 2, ... selling 100 each, one a day from January 1st
  cratchit::Project project(int sales) {
    std::string vouchers;
    for (int i = 1; i <= sales; ++i) {
      vouchers += std::format("#VER A {} 202501{:02} \"Sale {}\"\r\n{{\r\n#TRANS 1930 {{}} 100\r\n#TRANS 3010 {{}} -100\r\n}}\r\n",
                              i, i, i);
    }
    std::ofstream(documents / "test.se", std::ios::binary)
        << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
           "#KONTO 1930 \"Bank\"\r\n#KONTO 2099 \"Result\"\r\n#KONTO 3010 \"Sales\"\r\n"
           "#IB 0 1930 1000.00\r\n#IB 0 2099 -1000.00\r\n"
        << vouchers;
    auto const plan = cratchit::plan_project_from_sie(documents / "test.se");
    EXPECT_TRUE(plan) << plan.error();
    auto const created = cratchit::create_project_from_sie(*plan, "2026-11-09");
    EXPECT_TRUE(created) << created.error();
    return *created;
  }

  fs::path documents;
};

} // namespace

TEST_F(ReportsMenuTest, OffersTheFourReports) {
  Navigator navigator(cratchit::ui::make_reports_menu(project(1), {"2025-01-01", "2025-12-31"}));
  EXPECT_EQ(navigator.current().name(), "Rapporter");
  EXPECT_EQ(navigator.current().view(), (pyroceph::tui::View{"Rapporter för räkenskapsåret 2025."}));
  EXPECT_EQ(navigator.current().options(), (OptionsList{{U'1', "Balansrapport"},
                                                        {U'2', "Resultatrapport"},
                                                        {U'3', "Verifikationslista"},
                                                        {U'4', "Huvudbok"}}));
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::reports);
}

TEST_F(ReportsMenuTest, BalanceAndResultReportsShowTheBooksOfTheYear) {
  Navigator navigator(cratchit::ui::make_reports_menu(project(2), {"2025-01-01", "2025-12-31"}));
  ASSERT_TRUE(navigator.press(U'1'));
  EXPECT_EQ(navigator.current().name(), "Balansrapport");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::report);
  auto view = navigator.current().view();
  ASSERT_GE(view.size(), 3u);
  EXPECT_TRUE(view[0].starts_with("Balansrapport: rad 1–")) << view[0];
  EXPECT_EQ(view[1], "Balansrapport 2025-01-01 – 2025-12-31");
  EXPECT_EQ(view[2], "Test AB") << "the project's name";
  EXPECT_NE(std::ranges::find(view, "1930            1000.00         200.00        1200.00  Bank"), view.end());
  ASSERT_TRUE(navigator.back());
  ASSERT_TRUE(navigator.press(U'2'));
  EXPECT_EQ(navigator.current().name(), "Resultatrapport");
  view = navigator.current().view();
  EXPECT_NE(std::ranges::find(view, "                 200.00  Årets resultat"), view.end());
}

TEST_F(ReportsMenuTest, AReportLongerThanAPageIsTurnedInPlace) {
  // 30 vouchers of 6 lines each in the voucher list
  Navigator navigator(cratchit::ui::make_reports_menu(project(30), {"2025-01-01", "2025-12-31"}));
  ASSERT_TRUE(navigator.press(U'3'));
  EXPECT_EQ(navigator.current().name(), "Verifikationslista");
  auto const first = navigator.current().view();
  ASSERT_EQ(first.size(), 22u); // heading, 20 lines, "…"
  EXPECT_EQ(first[0], "Verifikationslista: rad 1–20 av 184");
  EXPECT_EQ(first[21], "  …");
  EXPECT_FALSE(navigator.current().on_key_in_place(U'<')) << "nothing before the first page";
  EXPECT_TRUE(navigator.current().on_key_in_place(U'>'));
  EXPECT_EQ(navigator.current().view()[0], "Verifikationslista: rad 21–40 av 184");
  EXPECT_TRUE(navigator.current().on_key_in_place(U'<'));
  EXPECT_EQ(navigator.current().view()[0], "Verifikationslista: rad 1–20 av 184");
}

TEST_F(ReportsMenuTest, HuvudbokTakesAnAccountNumberAndRefusesOthers) {
  Navigator navigator(cratchit::ui::make_reports_menu(project(2), {"2025-01-01", "2025-12-31"}));
  ASSERT_TRUE(navigator.press(U'4'));
  EXPECT_EQ(navigator.current().name(), "Huvudbok");
  EXPECT_EQ(navigator.current().help_topic(), cratchit::ui::help_topic::ledger_account);
  auto refused = navigator.current().on_input(U"Bank");
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "Skriv kontots nummer med bara siffror, till exempel 1930.");
  refused = navigator.current().on_input(U"4010");
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "Konto 4010 finns inte i bokföringen och har inga rader i 2025.");
  auto accepted = navigator.current().on_input(U"1930");
  ASSERT_TRUE(accepted) << accepted.error();
  ASSERT_TRUE(accepted->next);
  navigator.enter(std::move(accepted->next));
  EXPECT_EQ(navigator.current().name(), "Huvudbok 1930");
  auto const view = navigator.current().view();
  ASSERT_GE(view.size(), 2u);
  EXPECT_EQ(view[1], "Huvudbok konto 1930 Bank 2025-01-01 – 2025-12-31");
  EXPECT_EQ(view.back(), "Utgående balans                                             1200.00");
}
