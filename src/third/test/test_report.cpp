// The reports of a fiscal year (core/books/report), from take 3 (plaiground
// session 60cae9fe, test/test_report.cpp), in Swedish and with the trial
// balance split into Balansrapport and Resultatrapport
#include "core/books/import.hpp"
#include "core/books/report.hpp"

#include "sie_test_file.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using cratchit::books::Books;
using sie_test::balanced_balances;
using sie_test::parse_sie;
using sie_test::sale_and_cost;
using sie_test::sie_file;

namespace {

// 2025: a sale of 500 and bank costs of 10 (a profit of 490, not booked on
// equity); 2026: a sale of 100 and a voucher with a struck and an added row
Books books_2025_2026(std::string_view opening_2026 = "1490.00") {
  auto const first = cratchit::books::plan_import({}, {"Test", ""},
                                                  parse_sie(sie_file(2025, balanced_balances(), sale_and_cost(2025))));
  auto const second = cratchit::books::plan_import(
      first.books, first.identity,
      parse_sie(sie_file(2026, std::format("#IB 0 1930 {}\n#IB 0 2099 -1000\n", opening_2026),
                         "#VER A 1 20260110 \"Sale\"\n{\n#TRANS 1930 {} 100\n#TRANS 3010 {} -100\n}\n"
                         "#VER B 1 20260111 \"Fix\"\n{\n#TRANS 1930 {} 0\n#BTRANS 6570 {} 5 \"\" \"Wrong\" 0 \"Kim\"\n"
                         "#RTRANS 1930 {} -0 \"\" \"Added\" 0 \"Kim\"\n#TRANS 1930 {} -0\n}\n")));
  EXPECT_TRUE(second.errors.empty());
  return second.books;
}

// 2025 as above, with its result (490) booked on 2099 against 8999
Books books_with_result_booked() {
  auto const plan = cratchit::books::plan_import(
      {}, {"Test", ""},
      parse_sie(sie_file(2025, "#IB 0 1930 1000.00\n#IB 0 2099 -1000.00\n",
                         "#KONTO 8999 \"Result of the year\"\n" + sale_and_cost(2025) +
                             "#VER A 3 20251231 \"Result of the year\"\n{\n#TRANS 8999 {} 490\n"
                             "#TRANS 2099 {} -490\n}\n")));
  EXPECT_TRUE(plan.errors.empty());
  return plan.books;
}

std::string text(std::vector<std::string> const& lines) {
  std::string result;
  for (auto const& line : lines) result += line + "\n";
  return result;
}

} // namespace

TEST(Report, BalanceReportOfTheFirstYear) {
  EXPECT_EQ(text(cratchit::books::balance_report(books_2025_2026(), 0, "Test AB, 556000-0001")),
            "Balansrapport 2025-01-01 – 2025-12-31\n"
            "Test AB, 556000-0001\n"
            "\n"
            "Konto                IB     Förändring             UB  Namn\n"
            "\n"
            "Tillgångar\n"
            "1930            1000.00         490.00        1490.00  Bank\n"
            "                1000.00         490.00        1490.00  Summa tillgångar\n"
            "\n"
            "Eget kapital och skulder\n"
            "2099           -1000.00           0.00       -1000.00  Result\n"
            "               -1000.00           0.00       -1000.00  Summa eget kapital och skulder\n"
            "\n"
            "                   0.00        -490.00        -490.00  Beräknat resultat\n"
            "                   0.00           0.00           0.00  Summa\n"
            "\n"
            "Belopp: debet positivt, kredit negativt. Beräknat resultat är resultatkontonas summa (en vinst är negativ).\n"
            "Summa är noll när bokföringen går ihop.\n"
            "Ingående balanser är som källan angav dem (året före finns inte i bokföringen).\n");
}

TEST(Report, BalanceReportTellsOfOpeningBalancesThatDoNotAddUpAndThoseThatDifferFromTheSource) {
  // 2025's profit of 490 is not booked on equity, so 2026 opens 490 off; and
  // the source reported 1500 on 1930, where cratchit carries 1490
  auto const report = text(cratchit::books::balance_report(books_2025_2026("1500.00"), 1, "Test AB"));
  EXPECT_NE(report.find("1930            1490.00         100.00        1590.00  Bank\n"), std::string::npos) << report;
  EXPECT_NE(report.find("                 490.00           0.00         490.00  Summa\n"), std::string::npos) << report;
  EXPECT_NE(report.find("De ingående balanserna går inte ihop: 490.00. Oftast är föregående års resultat inte bokfört "
                        "på eget kapital än.\n"),
            std::string::npos)
      << report;
  EXPECT_NE(report.find("Ingående balanser är förda från de utgående balanserna 2025-12-31.\n"
                        "  1930: källan angav 1500.00, förd 1490.00 (skillnad -10.00)\n"),
            std::string::npos)
      << report;
}

TEST(Report, ResultReportInGroupsWithIncomePositive) {
  EXPECT_EQ(text(cratchit::books::result_report(books_2025_2026(), 0, "Test AB")),
            "Resultatrapport 2025-01-01 – 2025-12-31\n"
            "Test AB\n"
            "\n"
            "Konto            Belopp  Namn\n"
            "\n"
            "Rörelsens intäkter\n"
            "3010             500.00  Sales\n"
            "                 500.00  Summa rörelsens intäkter\n"
            "\n"
            "Övriga externa kostnader\n"
            "6570             -10.00  Bank costs\n"
            "                 -10.00  Summa övriga externa kostnader\n"
            "\n"
            "                 490.00  Årets resultat\n"
            "\n"
            "Belopp: intäkter positiva, kostnader negativa. Ett positivt resultat är en vinst.\n");
}

TEST(Report, ResultReportShowsTheBookedResultAfterTheYearsResult) {
  auto const report = text(cratchit::books::result_report(books_with_result_booked(), 0, "Test AB"));
  EXPECT_NE(report.find("                 490.00  Årets resultat\n"
                        "\n"
                        "Årets resultat bokfört (8990–8999)\n"
                        "8999            -490.00  Result of the year\n"
                        "                   0.00  Kvar efter resultatbokningen\n"),
            std::string::npos)
      << report;
}

TEST(Report, BalanceReportOfAYearWithItsResultBookedHasNoCalculatedResult) {
  auto const report = text(cratchit::books::balance_report(books_with_result_booked(), 0, "Test AB"));
  EXPECT_NE(report.find("2099           -1000.00        -490.00       -1490.00  Result\n"), std::string::npos) << report;
  EXPECT_NE(report.find("                   0.00           0.00           0.00  Beräknat resultat\n"
                        "                   0.00           0.00           0.00  Summa\n"),
            std::string::npos)
      << report;
}

TEST(Report, VoucherListMarksAddedAndStruckRows) {
  EXPECT_EQ(text(cratchit::books::voucher_list(books_2025_2026(), 1, "Test AB")),
            "Verifikationslista 2026-01-01 – 2026-12-31\n"
            "Test AB\n"
            "\n"
            "2 verifikationer\n"
            "\n"
            "A1  2026-01-10  Sale\n"
            "  Konto          Debet       Kredit\n"
            "  1930          100.00               Bank\n"
            "  3010                       100.00  Sales\n"
            "  Summa         100.00       100.00\n"
            "\n"
            "B1  2026-01-11  Fix\n"
            "  Konto          Debet       Kredit\n"
            "  1930            0.00               Bank\n"
            "  6570            5.00               Wrong  (struken av Kim)\n"
            "  1930            0.00               Added  (tillagd av Kim)\n"
            "  Summa           0.00         0.00\n");
}

TEST(Report, AccountLedgerWithRunningBalance) {
  auto const ledger = cratchit::books::account_ledger(books_2025_2026(), 0, "Test AB", "1930");
  ASSERT_TRUE(ledger.has_value());
  EXPECT_EQ(text(*ledger), "Huvudbok konto 1930 Bank 2025-01-01 – 2025-12-31\n"
                           "Test AB\n"
                           "\n"
                           "Datum       Verifikation          Debet       Kredit          Saldo  Text\n"
                           "Ingående balans                                             1000.00\n"
                           "2025-02-01  A1                   500.00                     1500.00  Sale\n"
                           "2025-03-01  A2                                 10.00        1490.00  Bank cost\n"
                           "Utgående balans                                             1490.00\n");
}

TEST(Report, LedgerOfAnUnknownAccountIsNothing) {
  auto const books = books_2025_2026();
  EXPECT_FALSE(cratchit::books::account_ledger(books, 0, "Test AB", "9999").has_value());
  EXPECT_TRUE(cratchit::books::account_ledger(books, 0, "Test AB", "2099").has_value()); // defined, no rows
}

TEST(Report, GeneralLedgerHasEveryAccountWithRowsOrAnOpeningBalance) {
  EXPECT_EQ(text(cratchit::books::general_ledger(books_2025_2026(), 0, "Test AB")),
            "Huvudbok 2025-01-01 – 2025-12-31\n"
            "Test AB\n"
            "\n"
            "Konto 1930 Bank\n"
            "Datum       Verifikation          Debet       Kredit          Saldo  Text\n"
            "Ingående balans                                             1000.00\n"
            "2025-02-01  A1                   500.00                     1500.00  Sale\n"
            "2025-03-01  A2                                 10.00        1490.00  Bank cost\n"
            "Utgående balans                                             1490.00\n"
            "\n"
            "Konto 2099 Result\n"
            "Datum       Verifikation          Debet       Kredit          Saldo  Text\n"
            "Ingående balans                                            -1000.00\n"
            "Utgående balans                                            -1000.00\n"
            "\n"
            "Konto 3010 Sales\n"
            "Datum       Verifikation          Debet       Kredit          Saldo  Text\n"
            "Ingående balans                                                0.00\n"
            "2025-02-01  A1                                500.00        -500.00  Sale\n"
            "Utgående balans                                             -500.00\n"
            "\n"
            "Konto 6570 Bank costs\n"
            "Datum       Verifikation          Debet       Kredit          Saldo  Text\n"
            "Ingående balans                                                0.00\n"
            "2025-03-01  A2                    10.00                       10.00  Bank cost\n"
            "Utgående balans                                               10.00\n");
}

TEST(Report, VoucherDocumentHasWhatTheLawAsksOfAVoucher) {
  auto const books = books_2025_2026();
  EXPECT_EQ(text(cratchit::books::voucher_document(books, 0, books.years[0].vouchers[0], "Test AB (556000-0001)")),
            "Verifikation A1\n"
            "Test AB (556000-0001)\n"
            "Räkenskapsår 2025-01-01 – 2025-12-31\n"
            "\n"
            "Serie:          A\n"
            "Datum:          2025-02-01\n"
            "Text:           Sale\n"
            "Belopp:         500.00\n"
            "Registrerad:    2025-02-05\n"
            "\n"
            "Konto          Debet       Kredit\n"
            "1930          500.00               Bank\n"
            "3010                       500.00  Sales\n"
            "Summa         500.00       500.00\n");
}

TEST(Report, VoucherDocumentMarksStruckAndAddedRowsWithTheirOwnText) {
  auto const books = books_2025_2026();
  auto const document = text(cratchit::books::voucher_document(books, 1, books.years[1].vouchers[1], "Test AB"));
  EXPECT_NE(document.find("6570            5.00               Bank costs  Wrong  (struken av Kim)\n"
                          "1930            0.00               Bank  Added  (tillagd av Kim)\n"
                          "Summa           0.00         0.00\n"),
            std::string::npos)
      << document;
}
