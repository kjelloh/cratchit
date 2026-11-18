#include "core/skv/vat_return/account_boxes.hpp"
#include "core/skv/vat_return/box.hpp"
#include "core/skv/vat_return/box_texts.hpp"
#include "core/skv/vat_return/checks.hpp"
#include "core/skv/vat_return/clearing.hpp"
#include "core/skv/vat_return/document.hpp"
#include "core/skv/vat_return/report.hpp"
#include "core/skv/vat_return/review.hpp"
#include "core/skv/vat_return/steps.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <vector>

using namespace cratchit;
using namespace cratchit::skv::vat_return;
using namespace std::chrono;

namespace {

books::Posting posting(std::string account, Amount amount, books::Posting::Kind kind = books::Posting::Kind::normal) {
  return {.kind = kind, .account = std::move(account), .amount = amount};
}

books::Voucher voucher(std::string series, std::string number, Date date, books::Postings postings) {
  return {.series = std::move(series),
          .number = std::move(number),
          .event = {"Event", 0, date},
          .registered = std::nullopt,
          .signature = {},
          .postings = std::move(postings)};
}

// Fiscal years 2025-05-01 - 2026-04-30 and 2026-05-01 - 2027-04-30, as a
// company whose year ends in April: the second quarter of 2026 spans both
books::Books two_years() {
  books::Books books;
  books.years.push_back({.start = 2025y / May / 1, .end = 2026y / April / 30});
  books.years.push_back({.start = 2026y / May / 1, .end = 2027y / April / 30});
  auto& first = books.years[0].vouchers;
  first.push_back(voucher("B", "1", 2026y / March / 31, {posting("1510", 12500), posting("3001", -10000),
                                                         posting("2611", -2500)}));
  first.push_back(voucher("B", "2", 2026y / April / 1, {posting("1510", 6250), posting("3001", -5000),
                                                        posting("2611", -1250)}));
  first.push_back(voucher("D", "1", 2026y / April / 30, {posting("2440", -1250), posting("6540", 1000),
                                                         posting("2641", 250)}));
  auto& second = books.years[1].vouchers;
  second.push_back(voucher("B", "1", 2026y / May / 2, {posting("1510", 2500), posting("3002", -2000),
                                                       posting("2611", -500, books::Posting::Kind::added),
                                                       posting("2621", -500, books::Posting::Kind::removed)}));
  second.push_back(voucher("D", "1", 2026y / July / 1, {posting("2440", -1250), posting("6540", 1000),
                                                        posting("2641", 250)}));
  return books;
}

} // namespace

TEST(VatReturn, TheBuiltInBoxesOfAccountsRead) {
  auto const boxes = account_boxes(account_boxes_text());
  ASSERT_TRUE(boxes.has_value()) << boxes.error();
  EXPECT_GT(boxes->size(), 100u);
}

TEST(VatReturn, BoxOfAnAccount) {
  EXPECT_EQ(box_of("3001"), "05");
  EXPECT_EQ(box_of("2611"), "10");
  EXPECT_EQ(box_of("2641"), "48");
  EXPECT_EQ(box_of("3305"), "40");
  EXPECT_EQ(box_of("2650"), std::nullopt); // a source of box 49, not added up into it
  EXPECT_EQ(box_of("1930"), std::nullopt);
  EXPECT_EQ(box_of(""), std::nullopt);
}

TEST(VatReturn, AccountBoxesText) {
  auto const boxes = account_boxes("\xEF\xBB\xBF" "3001;05;MP1\r\n\r\n2611;10\n2641;48;I");
  ASSERT_TRUE(boxes.has_value()) << boxes.error();
  EXPECT_EQ(*boxes, (std::map<std::string, Box>{{"2611", "10"}, {"2641", "48"}, {"3001", "05"}}));
}

TEST(VatReturn, AccountBoxesTextWithHeadingAndOutColumn) {
  auto const boxes = account_boxes("in;vat;out;hint\r\n3001;05;;MP1\r\n;49;2650;R2\r\n");
  ASSERT_TRUE(boxes.has_value()) << boxes.error();
  EXPECT_EQ(*boxes, (std::map<std::string, Box>{{"3001", "05"}}));
}

TEST(VatReturn, AccountBoxesErrorsNameTheLine) {
  EXPECT_EQ(account_boxes("3001;05;MP1\n301;05;MP1\n").error(), "Line 2: the account is not four digits: 301;05;MP1");
  EXPECT_EQ(account_boxes("3001;5;MP1\n").error(), "Line 1: the box is not two digits: 3001;5;MP1");
  EXPECT_EQ(account_boxes("3001\n").error(), "Line 1: the account is not four digits: 3001");
  EXPECT_EQ(account_boxes("3001;05;MP1\n\n3001;06;X\n").error(), "Line 3: account 3001 is listed before");
}

TEST(VatReturn, CalendarQuarters) {
  EXPECT_EQ(quarter_of(2026y, 1), (Period{2026y / January / 1, 2026y / March / 31}));
  EXPECT_EQ(quarter_of(2026y, 2), (Period{2026y / April / 1, 2026y / June / 30}));
  EXPECT_EQ(quarter_of(2026y, 3), (Period{2026y / July / 1, 2026y / September / 30}));
  EXPECT_EQ(quarter_of(2026y, 4), (Period{2026y / October / 1, 2026y / December / 31}));
  EXPECT_EQ(quarter_of(2026y, 0), std::nullopt);
  EXPECT_EQ(quarter_of(2026y, 5), std::nullopt);
}

TEST(VatReturn, QuartersEndingInAFiscalYear) {
  EXPECT_EQ(quarters_ending_in(2026y / January / 1, 2026y / December / 31),
            (std::vector<Period>{*quarter_of(2026y, 1), *quarter_of(2026y, 2), *quarter_of(2026y, 3),
                                 *quarter_of(2026y, 4)}));
  // A year ending in April holds the quarter ending in June, but not the one it ends in
  EXPECT_EQ(quarters_ending_in(2026y / May / 1, 2027y / April / 30),
            (std::vector<Period>{*quarter_of(2026y, 2), *quarter_of(2026y, 3), *quarter_of(2026y, 4),
                                 *quarter_of(2027y, 1)}));
  EXPECT_TRUE(quarters_ending_in(2026y / April / 1, 2026y / May / 31).empty());
}

TEST(VatReturn, QuarterHoldingADate) {
  EXPECT_EQ(quarter_holding(2026y / January / 1), *quarter_of(2026y, 1));
  EXPECT_EQ(quarter_holding(2026y / June / 30), *quarter_of(2026y, 2));
  EXPECT_EQ(quarter_holding(2026y / October / 27), *quarter_of(2026y, 4));
  EXPECT_EQ(quarter_holding(2026y / December / 31), *quarter_of(2026y, 4));
}

TEST(VatReturn, QuarterName) {
  EXPECT_EQ(quarter_name(*quarter_of(2026y, 1)), "2026 Q1");
  EXPECT_EQ(quarter_name(*quarter_of(2026y, 3)), "2026 Q3");
}

TEST(VatReturn, AQuarterAcrossTwoFiscalYears) {
  auto const period = *quarter_of(2026y, 2);
  auto const document = document_of(two_years(), period);
  EXPECT_EQ(document.period, period);
  EXPECT_EQ(document.boxes,
            (std::vector<BoxAmount>{
                {"05", 7000,
                 {{"3001", -5000, {{2025y / May / 1, "B", "2", 2026y / April / 1, -5000}}},
                  {"3002", -2000, {{2026y / May / 1, "B", "1", 2026y / May / 2, -2000}}}}},
                {"10", 1700, // the form figure, truncated: the account keeps -17.50
                 {{"2611", -1750,
                   {{2025y / May / 1, "B", "2", 2026y / April / 1, -1250},
                    {2026y / May / 1, "B", "1", 2026y / May / 2, -500}}}}},
                {"48", 200, {{"2641", 250, {{2025y / May / 1, "D", "1", 2026y / April / 30, 250}}}}},
            }));
}

TEST(VatReturn, VatToPayAddsUpBox49AsTheForm) {
  // Output VAT 250 + 60 + 25, input VAT 100, all positive on the form; sales box 05 not counted
  Document const document{*quarter_of(2026y, 1),
                          {{"05", 100000, {}}, {"10", 25000, {}}, {"11", 6000, {}}, {"30", 2500, {}},
                           {"48", 10000, {}}}};
  EXPECT_EQ(vat_to_pay(document), 23500);
  Document const back{*quarter_of(2026y, 1), {{"10", 2500, {}}, {"48", 10000, {}}}};
  EXPECT_EQ(vat_to_pay(back), -7500);
  EXPECT_EQ(vat_to_pay(Document{*quarter_of(2026y, 1), {}}), 0);
}

TEST(VatReturn, NoVouchersInThePeriodGiveNoBoxes) {
  auto const document = document_of(two_years(), *quarter_of(2025y, 4));
  EXPECT_TRUE(document.boxes.empty());
}

namespace {

// A year of quarterly VAT: Q1 to pay 250 (cleared M 1 with rounding, charged in May), Q2 to get back 120
// (cleared M 2 to 1650, credited in August), Q3 not cleared; a payment to the tax account and a charge that
// matches nothing
books::Books steps_year() {
  books::Books books;
  books.years.push_back({.start = 2026y / January / 1, .end = 2026y / December / 31});
  auto& v = books.years[0].vouchers;
  v.push_back(voucher("B", "1", 2026y / February / 10, {posting("1510", 125040), posting("3001", -100032),
                                                        posting("2611", -25008)}));
  v.push_back(voucher("M", "1", 2026y / March / 31, {posting("2611", 25008), posting("2650", -25000),
                                                     posting("3740", -8)}));
  v.push_back(voucher("D", "1", 2026y / April / 3, {posting("2440", -60000), posting("4010", 48000),
                                                    posting("2641", 12000)}));
  v.push_back(voucher("A", "1", 2026y / May / 12, {posting("2650", 25000), posting("1630", -25000)}));
  v.push_back(voucher("M", "2", 2026y / June / 30, {posting("2641", -12000), posting("1650", 12000)}));
  v.push_back(voucher("A", "2", 2026y / July / 3, {posting("1630", 50000), posting("1930", -50000)}));
  v.push_back(voucher("A", "3", 2026y / August / 12, {posting("1630", 12000), posting("1650", -12000)}));
  v.push_back(voucher("A", "4", 2026y / August / 13, {posting("2650", 999), posting("1630", -999)}));
  v.push_back(voucher("B", "2", 2026y / September / 1, {posting("1510", 5000), posting("3001", -4000),
                                                        posting("2611", -1000),
                                                        posting("2650", -1000, books::Posting::Kind::removed)}));
  return books;
}

} // namespace

TEST(VatReturn, ClearingAccountsAndVatAccounts) {
  EXPECT_TRUE(is_vat_clearing_account(vat_liability_account));
  EXPECT_TRUE(is_vat_clearing_account(vat_receivable_account));
  EXPECT_FALSE(is_vat_clearing_account("2611"));
  EXPECT_TRUE(is_vat_account("2611"));
  EXPECT_TRUE(is_vat_account("2641"));
  EXPECT_FALSE(is_vat_account("2650"));
  EXPECT_FALSE(is_vat_account("3001"));
}

TEST(VatReturn, StepsOfQuarters) {
  auto const steps = steps_of(steps_year(), quarters_ending_in(2026y / January / 1, 2026y / December / 31));
  ASSERT_EQ(steps.periods.size(), 4u);
  auto const& q1 = steps.periods[0];
  EXPECT_EQ(q1.period_vat, -25008);
  EXPECT_EQ(q1.cleared, -25000);
  EXPECT_EQ(q1.clearings, (std::vector<VoucherPosting>{{2026y / January / 1, "M", "1", 2026y / March / 31, -25000}}));
  EXPECT_EQ(q1.charged, -25000);
  EXPECT_EQ(q1.charges, (std::vector<VoucherPosting>{{2026y / January / 1, "A", "1", 2026y / May / 12, -25000}}));
  auto const& q2 = steps.periods[1];
  EXPECT_EQ(q2.period_vat, 12000);
  EXPECT_EQ(q2.cleared, 12000);
  EXPECT_EQ(q2.charged, 12000);
  ASSERT_EQ(q2.charges.size(), 1u);
  EXPECT_EQ(q2.charges[0].number, "3");
  // A removed posting on 2650 does not make B 2 a clearing voucher
  auto const& q3 = steps.periods[2];
  EXPECT_EQ(q3.period_vat, -1000);
  EXPECT_EQ(q3.cleared, 0);
  EXPECT_TRUE(q3.clearings.empty());
  EXPECT_EQ(q3.charged, 0);
  EXPECT_EQ(steps.periods[3], (PeriodSteps{*quarter_of(2026y, 4), 0, 0, {}, 0, {}}));
  // A 2 pays into the tax account and is no step; A 4 matches no cleared quarter
  EXPECT_EQ(steps.unmatched_charges,
            (std::vector<VoucherPosting>{{2026y / January / 1, "A", "4", 2026y / August / 13, -999}}));
}

TEST(VatReturn, AChargeGoesToTheOldestQuarterClearedByItsAmountAndOnlyOnce) {
  books::Books books;
  books.years.push_back({.start = 2026y / January / 1, .end = 2026y / December / 31});
  auto& v = books.years[0].vouchers;
  v.push_back(voucher("B", "1", 2026y / February / 1, {posting("1510", 500), posting("2611", -500)}));
  v.push_back(voucher("M", "1", 2026y / March / 31, {posting("2611", 500), posting("2650", -500)}));
  v.push_back(voucher("B", "2", 2026y / May / 1, {posting("1510", 500), posting("2611", -500)}));
  v.push_back(voucher("M", "2", 2026y / June / 30, {posting("2611", 500), posting("2650", -500)}));
  // Charged before the second quarter starts: only the first can take it
  v.push_back(voucher("A", "1", 2026y / March / 31, {posting("2650", 500), posting("1630", -500)}));
  v.push_back(voucher("A", "2", 2026y / August / 12, {posting("2650", 500), posting("1630", -500)}));
  v.push_back(voucher("A", "3", 2026y / November / 12, {posting("2650", 500), posting("1630", -500)}));
  auto const steps = steps_of(books, quarters_ending_in(2026y / January / 1, 2026y / December / 31));
  EXPECT_EQ(steps.periods[0].charges[0].number, "1");
  EXPECT_EQ(steps.periods[1].charges[0].number, "2");
  ASSERT_EQ(steps.unmatched_charges.size(), 1u);
  EXPECT_EQ(steps.unmatched_charges[0].number, "3");
}

TEST(VatReturn, AChargeFindsItsQuarterInTheYearBefore) {
  auto books = two_years();
  // Q1 2026 (in 2025/26) cleared in April, charged in the next fiscal year
  books.years[0].vouchers.push_back(voucher("M", "1", 2026y / March / 31, {posting("2611", 2500), posting("2650", -2500)}));
  books.years[1].vouchers.push_back(voucher("A", "1", 2026y / May / 12, {posting("2650", 2500), posting("1630", -2500)}));
  auto const steps = steps_of(books, quarters_ending_in(2025y / May / 1, 2027y / April / 30));
  auto const q1 = std::ranges::find(steps.periods, *quarter_of(2026y, 1), &PeriodSteps::period);
  ASSERT_NE(q1, steps.periods.end());
  EXPECT_EQ(q1->cleared, -2500);
  EXPECT_EQ(q1->charged, -2500);
  EXPECT_EQ(q1->charges[0].fiscal_year, 2026y / May / 1);
  EXPECT_TRUE(steps.unmatched_charges.empty());
}

TEST(VatReturn, StepsRow) {
  EXPECT_EQ(steps_heading(), "      Period     Cleared     Charged");
  PeriodSteps const steps{*quarter_of(2026y, 1), -25008, -25000,
                          {{2026y / January / 1, "M", "1", 2026y / March / 31, -25000}}, 0, {}};
  EXPECT_EQ(steps_row(steps), "     -250.08     -250.00        0.00  cleared by M1");
}

TEST(VatReturn, ReportBox49IsMinusBox48WithOnlyInputVat) {
  Document const document{*quarter_of(2026y, 2), {{"48", 12000, {}}}};
  PeriodSteps const steps{*quarter_of(2026y, 2), 12000, 0, {}, 0, {}};
  auto const lines = report_of(books::Books{}, document, steps, "Test AB", std::nullopt);
  EXPECT_NE(std::ranges::find(lines, "48                                          120.00  Ingående moms att dra av"),
            lines.end());
  EXPECT_NE(std::ranges::find(lines, "Beräkning av moms"), lines.end());
  EXPECT_NE(std::ranges::find(lines, "49                                         -120.00  10 + 11 + 12 + 30 + 31 + 32 + 60 + 61 + 62 - 48"),
            lines.end());
  EXPECT_EQ(lines.back(), "Ruta 49 på blanketten: 120.00 att få tillbaka.");
}

TEST(VatReturn, ClearingToPayBooksTheOreOnTheRoundingAccount) {
  // Box 10 2611 -250.50 (form 250), box 48 2641 100.30 (form 100): 150 to pay, 0.20 left
  Document const document{*quarter_of(2026y, 2),
                          {{"05", 100000, {{"3001", -100000, {}}}},
                           {"10", 25000, {{"2611", -25050, {}}}},
                           {"48", 10000, {{"2641", 10030, {}}}}}};
  auto const clearing = clearing_of(document);
  ASSERT_TRUE(clearing);
  EXPECT_EQ(clearing->event, (books::Event{"Momsrapport 2026-04-01 - 2026-06-30", 25050, 2026y / June / 30}));
  EXPECT_EQ(clearing->postings, (books::Postings{posting("2611", 25050), posting("2641", -10030),
                                                 posting("2650", -15000), posting("3740", -20)}));
}

TEST(VatReturn, ClearingToGetBackDebitsTheReceivable) {
  Document const document{*quarter_of(2026y, 3), {{"48", 12000, {{"2641", 12030, {}}}}}};
  auto const clearing = clearing_of(document);
  ASSERT_TRUE(clearing);
  EXPECT_EQ(clearing->postings, (books::Postings{posting("2641", -12030), posting("1650", 12000), posting("3740", 30)}));
  EXPECT_EQ(clearing->event.amount, 12030);
}

TEST(VatReturn, ClearingOfNothingToPayHasNoBox49Row) {
  Document const document{*quarter_of(2026y, 3),
                          {{"10", 10000, {{"2611", -10050, {}}}}, {"48", 10000, {{"2641", 10050, {}}}}}};
  auto const clearing = clearing_of(document);
  ASSERT_TRUE(clearing);
  EXPECT_EQ(clearing->postings, (books::Postings{posting("2611", 10050), posting("2641", -10050)}));
}

TEST(VatReturn, ClearingNeedsAVatAccountThatChanged) {
  EXPECT_FALSE(clearing_of(Document{*quarter_of(2026y, 3), {{"05", 100000, {{"3001", -100000, {}}}}}}));
  EXPECT_FALSE(clearing_of(Document{*quarter_of(2026y, 3), {{"10", 0, {{"2611", 0, {}}}}}}));
}

TEST(VatReturn, ClearingOfTheBooksBalances) {
  // Box 10 2611 -17.50 (form 17), box 48 2641 2.50 (form 2): 15 to pay, the öre cancel out
  auto const document = document_of(two_years(), *quarter_of(2026y, 2));
  auto const clearing = clearing_of(document);
  ASSERT_TRUE(clearing);
  EXPECT_EQ(clearing->postings, (books::Postings{posting("2611", 1750), posting("2641", -250), posting("2650", -1500)}));
  EXPECT_EQ(books::debit(clearing->postings), books::credit(clearing->postings));
  EXPECT_EQ(clearing->event.amount, 1750);
}

namespace {

Period const q3_2026{2026y / July / 1, 2026y / September / 30};

// The second fiscal year of two_years with one voucher added; its registration day given
books::Books with(books::Voucher added, std::optional<Date> registered = std::nullopt) {
  auto books = two_years();
  added.registered = registered;
  books.years[1].vouchers.push_back(std::move(added));
  return books;
}

std::string all_of(std::vector<std::string> const& texts) {
  std::string result;
  for (auto const& text : texts) {
    result += text + "\n";
  }
  return result;
}

} // namespace

TEST(VatReturnChecks, RatesOfTheOutputVatBoxes) {
  EXPECT_EQ(rate_of("10"), 25);
  EXPECT_EQ(rate_of("31"), 12);
  EXPECT_EQ(rate_of("62"), 6);
  EXPECT_EQ(rate_of("48"), std::nullopt);
}

TEST(VatReturnChecks, NothingFoundInBooksThatAreRight) {
  EXPECT_TRUE(checks_of(two_years(), q3_2026).empty());
  // A reverse charge purchase booked in full: base 9021 in box 20, 25 % output VAT in box 30, deducted in box 48
  auto const right = with(voucher("A", "1", 2026y / September / 10,
                                  {posting("1920", -12400), posting("6110", 12400), posting("2614", -3100),
                                   posting("2645", 3100), posting("9021", 12400), posting("9099", -12400)}));
  EXPECT_TRUE(checks_of(right, q3_2026).empty()) << all_of(checks_of(right, q3_2026));
}

TEST(VatReturnChecks, AnUnbookedPeriod) {
  auto books = two_years();
  books.years[1].place_draft({{"Loopia", 31125, 2026y / August / 31}, 2026y / October / 25});
  books.years[1].place_draft({{"Fortnox", 92300, 2026y / August / 7}, 2026y / October / 25});
  books.years[1].place_draft({{"Later", 100, 2026y / October / 3}, 2026y / October / 25});
  auto const findings = checks_of(books, q3_2026);
  EXPECT_EQ(findings, (std::vector<std::string>{
                          "2 utkast daterade i perioden, inte bokförda än (det första 2026-08-07 \"Fortnox\")"}));
}

TEST(VatReturnChecks, VatClearingAccountsInAPurchase) {
  // As A 48 2026: the reverse charge VAT against 2650 instead of 2614
  auto const books = with(voucher("A", "48", 2026y / September / 10,
                                  {posting("1920", -15700), posting("6110", 15700), posting("2650", -3100),
                                   posting("2645", 3100), posting("9021", 12400), posting("9099", -12400)}));
  auto const findings = checks_of(books, q3_2026);
  EXPECT_EQ(findings, (std::vector<std::string>{
                          "A48 2026-09-10 \"Event\": konterar 2650 eller 1650 tillsammans med 1920, 6110; bara en "
                          "momsavräkning eller en debitering från Skatteverket konterar dem"}));
}

TEST(VatReturnChecks, ReverseChargeInPairs) {
  auto const findings_of = [](books::Postings postings) {
    return checks_of(with(voucher("A", "13", 2026y / September / 10, std::move(postings))), q3_2026);
  };
  // The base without output VAT (the VAT left out)
  EXPECT_EQ(findings_of({posting("1920", -20468), posting("6540", 20468), posting("9022", 20468),
                         posting("9099", -20468)}),
            (std::vector<std::string>{"A13 2026-09-10 \"Event\": inköpsunderlag 204.68 i ruta 50, men ingen utgående moms "
                                      "i rutorna 60-62"}));
  // Output VAT without a base, and not deducted
  EXPECT_EQ(findings_of({posting("1920", -5117), posting("6540", 10234), posting("2614", -5117)}),
            (std::vector<std::string>{"A13 2026-09-10 \"Event\": utgående moms 51.17 i ruta 30, men inget inköpsunderlag "
                                      "i rutorna 20-24",
                                      "A13 2026-09-10 \"Event\": ruta 48 drar av 0.00, men den utgående momsen på inköpen "
                                      "är 51.17; den dras av i sin helhet"}));
  // The wrong rate: 25 % of 204.68 is 51.17 (towards zero)
  EXPECT_EQ(findings_of({posting("1920", -20468), posting("6540", 20468), posting("2614", -5100), posting("2645", 5100),
                         posting("9021", 20468), posting("9099", -20468)}),
            (std::vector<std::string>{"A13 2026-09-10 \"Event\": ruta 30 har 51.00, men 25 % av inköpsunderlaget 204.68 "
                                      "är 51.17"}));
  // 12 % rounds towards zero: 12 % of 100.05 is 12.006, so 12.00
  EXPECT_TRUE(findings_of({posting("1920", -10005), posting("6540", 10005), posting("2624", -1200),
                           posting("2645", 1200), posting("9021", 10005), posting("9099", -10005)})
                  .empty());
}

TEST(VatReturnChecks, RegisteredAfterTheClearing) {
  auto books = with(voucher("M", "2", 2026y / September / 30, {posting("2641", -250), posting("2650", 250)}),
                    2026y / October / 12);
  auto late = voucher("I", "6", 2026y / September / 20, {posting("2440", -1250), posting("6540", 1000),
                                                         posting("2641", 250)});
  late.registered = 2026y / October / 20;
  books.years[1].vouchers.push_back(late);
  auto early = late;
  early.number = "5";
  early.registered = 2026y / October / 1;
  books.years[1].vouchers.push_back(early);
  EXPECT_EQ(checks_of(books, q3_2026),
            (std::vector<std::string>{"I6 2026-09-20 \"Event\": registrerad 2026-10-20, efter momsavräkningen M2 "
                                      "(registrerad 2026-10-12); momsdeklarationen som gjordes då saknar den"}));
}

TEST(VatReturnChecks, VatAccountsOutsideTheReturn) {
  auto const books = with(voucher("A", "2", 2026y / August / 3, {posting("1920", -100), posting("2655", 100)}));
  EXPECT_EQ(checks_of(books, q3_2026),
            (std::vector<std::string>{"2655 har konteringar i perioden (A2), men redovisas inte i någon ruta"}));
}

TEST(VatReturnBoxTexts, TheBuiltInTextsRead) {
  auto const texts = box_texts(box_texts_text());
  ASSERT_TRUE(texts.has_value()) << texts.error();
  EXPECT_EQ(texts->front().box, "05");
  EXPECT_EQ(texts->front().section, "Momspliktig försäljning eller uttag");
  EXPECT_EQ(texts->front().title, "Momspliktig försäljning som inte ingår i ruta 06, 07 eller 08");
  EXPECT_TRUE(texts->front().description.starts_with("Momspliktig försäljning som inte ingår i ruta 06, 07 eller 08\nHär "))
      << texts->front().description;
  EXPECT_EQ(texts->front().description.find('\r'), std::string::npos);
  EXPECT_EQ(text_of("30")->title, "Utgående moms 25%");
  EXPECT_EQ(text_of("30")->section, "Momspliktiga inköp vid omvänd betalningsskyldighet");
  EXPECT_EQ(text_of("48")->title, "Ingående moms att dra av");
  EXPECT_EQ(text_of("48", -100)->title, "Tidigare avdragen moms att betala tillbaka");
  EXPECT_EQ(text_of("99"), std::nullopt);
  // The form's order: sales, reverse charge, import, exempt sales, then the VAT
  EXPECT_LT(form_position("32"), form_position("50"));
  EXPECT_LT(form_position("62"), form_position("35"));
  EXPECT_LT(form_position("42"), form_position("48"));
  EXPECT_EQ(form_position("99"), texts->size());
}

TEST(VatReturnBoxTexts, TextsOfAFile) {
  auto const texts = box_texts("\xEF\xBB\xBF" "Box;Section;Section;Description\r\n"
                               "10;Sales;Output 25%;\"Long\r\ntext; with a separator\"\r\n"
                               ";;;\r\n");
  ASSERT_TRUE(texts.has_value()) << texts.error();
  EXPECT_EQ(*texts, (std::vector<BoxText>{{"10", "Sales", "Output 25%", "Long\ntext; with a separator"}}));
  EXPECT_EQ(box_texts("1;Sales;Output\n").error(), "Row 1: the box is not two digits: 1");
  EXPECT_EQ(box_texts("Box;Section\n10;Sales;\n").error(), "Row 2: box 10 has no section or title");
}

TEST(VatReturnBoxTexts, ALongTitleIsCutInTheReport) {
  Document const document{*quarter_of(2026y, 2), {{"24", 10000, {}}}};
  auto const lines = report_of(books::Books{}, document, {}, "Test AB", std::nullopt);
  // Övriga inköp av tjänster i Sverige som köparen är betalningsskyldig för: 45 characters with ..., å and ö as one
  EXPECT_NE(std::ranges::find(lines, "24                                          100.00  Övriga inköp av tjänster i Sverige som köp..."),
            lines.end());
}

TEST(VatReturnReview, WrapAndCut) {
  EXPECT_EQ(wrap_text("aa bb cc", 5), (std::vector<std::string>{"aa bb", "cc"}));
  EXPECT_EQ(wrap_text("x\n\ny", 5), (std::vector<std::string>{"x", "", "y"}));
  EXPECT_EQ(wrap_text("a verylongword b", 4), (std::vector<std::string>{"a", "verylongword", "b"}));
  EXPECT_EQ(wrap_text("åäö åäö", 7), (std::vector<std::string>{"åäö åäö"})); // characters, not bytes
  EXPECT_EQ(cut_text("åäöåäö", 6), "åäöåäö");
  EXPECT_EQ(cut_text("åäöåäöx", 6), "åäö...");
}

namespace {

// Box 20 (base) and 48 (input VAT) of a reverse charge purchase booked without its output VAT, as A 48 2026
Document a48_like() {
  VoucherPosting const a48{2025y / May / 1, "A", "48", 2026y / March / 31, 0};
  auto base = a48;
  base.amount = 12400;
  auto vat = a48;
  vat.amount = 3100;
  return {*quarter_of(2026y, 1),
          {{"20", 12400, {{"9021", 12400, {base}}}}, {"48", 3100, {{"2645", 3100, {vat}}}}}};
}

} // namespace

TEST(VatReturnReview, EveryBoxOfTheForm) {
  auto const boxes = review_boxes(a48_like());
  std::vector<std::string> numbers;
  for (auto const& box : boxes) {
    numbers.push_back(box.box);
  }
  // In the form's order, box 49 aside (it is added up, not posted to)
  EXPECT_EQ(numbers, (std::vector<std::string>{"05", "06", "07", "08", "10", "11", "12", "20", "21", "22",
                                               "23", "24", "30", "31", "32", "50", "60", "61", "62", "35",
                                               "36", "37", "38", "39", "40", "41", "42", "48"}));
  auto const of = [&](std::string const& number) { return *std::ranges::find(boxes, number, &ReviewBox::box); };
  EXPECT_EQ(of("20"), (ReviewBox{"20", "Momspliktiga inköp vid omvänd betalningsskyldighet",
                                 "Inköp av varor från annat EU-land", 1}));
  EXPECT_EQ(of("30").postings, 0u); // the missing output VAT shows
  EXPECT_EQ(of("05").postings, 0u); // and so does every box of a section without postings
  EXPECT_EQ(of("48").title, "Ingående moms att dra av"); // the main title, whatever the sign
  EXPECT_EQ(review_boxes({}).size(), 28u); // a quarter without postings has every box too
}

TEST(VatReturnReview, ABoxWithItsPostingsAndDescription) {
  books::Books books;
  books.accounts.push_back({"4515", "Inköp av varor från annat EU-land, 25 %", "", "", ""});
  books.accounts.push_back({"9021", "Varuvärde Inköp annat EG-land", "", "", ""});
  books.years.push_back({.start = 2025y / May / 1, .end = 2026y / April / 30});
  books.years[0].vouchers.push_back(voucher("A", "48", 2026y / March / 31, {}));
  books.years[0].vouchers[0].event.heading = "Kortköp LAMYSHOP";
  auto const lines = review_of(books, a48_like(), "20");
  ASSERT_GT(lines.size(), 11u);
  EXPECT_EQ(std::vector<std::string>(lines.begin(), lines.begin() + 11),
            (std::vector<std::string>{"Ruta 20  Inköp av varor från annat EU-land",
                                      "Avsnitt: Momspliktiga inköp vid omvänd betalningsskyldighet",
                                      "På blanketten: 124.00",
                                      "Konton och verifikationer som i bokföringen (debet positivt, kredit negativt):",
                                      "  9021           124.00  Varuvärde Inköp annat EG-land",
                                      "        A48     2026-03-31         124.00  Kortköp LAMYSHOP",
                                      "Andra konton som redovisas i ruta 20, utan konteringar:",
                                      "  4515  Inköp av varor från annat EU-land, 25 %",
                                      "  4516  (finns inte i kontoplanen)",
                                      "  4517  (finns inte i kontoplanen)",
                                      ""}));
  EXPECT_EQ(lines[11], "Skatteverkets beskrivning av ruta 20:");
  EXPECT_EQ(lines.back(), "");
  EXPECT_TRUE(std::ranges::all_of(lines, [](std::string const& line) {
    return std::ranges::count_if(line, [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; }) <= 80;
  }));
  // A box without postings: its accounts and description
  // Box 48: both titles and both descriptions
  auto const vat = review_of(books, a48_like(), "48");
  EXPECT_EQ(vat[0], "Ruta 48  Ingående moms att dra av");
  EXPECT_EQ(vat[1], "Även: Tidigare avdragen moms att betala tillbaka");
  EXPECT_TRUE(std::ranges::any_of(vat, [](std::string const& line) {
    return line.starts_with("Välj alternativet Tidigare avdragen moms att betala tillbaka");
  })); // the second title's description follows the first
  auto const empty = review_of(books, a48_like(), "30");
  EXPECT_EQ(empty[2], "Inga konteringar 2026-01-01 – 2026-03-31.");
  EXPECT_EQ(empty[3], "Konton som redovisas i ruta 30:");
  EXPECT_EQ(empty[4], "  2614  (finns inte i kontoplanen)");
  // One account a line
  auto const sales = review_of(books, a48_like(), "05");
  EXPECT_EQ(sales[3], "Konton som redovisas i ruta 05:");
  EXPECT_EQ(sales[4], "  3001  (finns inte i kontoplanen)");
  EXPECT_EQ(sales[5], "  3002  (finns inte i kontoplanen)");
}

TEST(VatReturn, TheReturnWithoutThePeriodsOwnClearing) {
  books::Books books;
  books.years.push_back({.start = 2026y / January / 1, .end = 2026y / December / 31});
  auto& v = books.years[0].vouchers;
  v.push_back(voucher("D", "1", 2026y / May / 10, {posting("2440", -1250), posting("6540", 1000), posting("2641", 250)}));
  // A correction in the period: a clearing, but not the period's own
  v.push_back(voucher("A", "1", 2026y / May / 12, {posting("2641", -100), posting("2650", 100)}));
  // A purchase with its VAT against 2650: not a clearing, it counts
  v.push_back(voucher("A", "2", 2026y / May / 20, {posting("1920", -1250), posting("6110", 1000), posting("2641", 250),
                                                   posting("2650", 0)}));
  // The period's own clearing, with a memo account as Fortnox writes them
  v.push_back(voucher("M", "1", 2026y / June / 30, {posting("2641", -400), posting("1650", 400), posting("9099", 0)}));
  auto const period = *quarter_of(2026y, 2);
  auto const box_48 = [](Document const& document) {
    auto const found = std::ranges::find(document.boxes, Box("48"), &BoxAmount::box);
    return found == document.boxes.end() ? Amount{0} : found->amount;
  };
  EXPECT_EQ(box_48(document_of(books, period)), 0);                       // as in the books: cleared
  EXPECT_EQ(box_48(document_of(books, period, Clearings::left_out)), 400); // 250 - 100 + 250
}
