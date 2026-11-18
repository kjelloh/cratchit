#include "core/books/import.hpp"

#include "core/path_value.hpp"
#include "sie_test_file.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
using namespace std::chrono;
using cratchit::books::Books;
using cratchit::books::Identity;
using cratchit::books::ImportPlan;
using cratchit::books::plan_import;
using sie_test::balanced_balances;
using sie_test::parse_sie;
using sie_test::sale_and_cost;
using sie_test::sie_file;

namespace {

bool has(std::vector<std::string> const& texts, std::string const& text) {
  return std::ranges::find(texts, text) != texts.end();
}

std::string all(std::vector<std::string> const& texts) {
  std::string result;
  for (auto const& text : texts) {
    result += text + "\n";
  }
  return result;
}

Identity const fresh{"Test", ""};
Identity const known{"Test AB", "556000-0001"};

// The books after importing the files in order, each of which must import without errors
Books imported(std::vector<std::string> const& files) {
  Books books;
  Identity identity = fresh;
  for (auto const& file : files) {
    auto const plan = plan_import(books, identity, parse_sie(file));
    EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
    books = plan.books;
    identity = plan.identity;
  }
  return books;
}

std::string file_2025() {
  return sie_file(2025, balanced_balances(), sale_and_cost(2025));
}

// The 2026 file: opens with 2025's closing balances, sells 100 more
std::string file_2026(std::string_view opening_1930 = "1490.00") {
  return sie_file(2026,
                  std::format("#UB -1 1930 1490.00\n#UB -1 2099 -1000.00\n#RES -1 3010 -500\n#RES -1 6570 10\n"
                              "#IB 0 1930 {0}\n#IB 0 2099 -1000.00\n#UB 0 1930 {1}\n#UB 0 2099 -1000.00\n"
                              "#RES 0 3010 -100\n",
                              opening_1930, cratchit::format_amount(*cratchit::parse_amount(opening_1930) + 10000)),
                  "#VER A 1 20260110 \"Sale\"\n{\n#TRANS 1930 {} 100\n#TRANS 3010 {} -100\n}\n");
}

} // namespace

TEST(Import, FirstImportSetsIdentityAndAddsTheYear) {
  auto const plan = plan_import({}, fresh, parse_sie(file_2025()));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_TRUE(plan.identity_changed);
  EXPECT_EQ(plan.identity.orgnr, "556000-0001");
  EXPECT_EQ(plan.identity.name, "Test AB");
  EXPECT_EQ(plan.notes, (std::vector<std::string>{"The project gets organisation number 556000-0001 (#ORGNR)",
                                                  "The project's name changes from \"Test\" to \"Test AB\" (#FNAMN)"}));
  EXPECT_EQ(plan.year_start, year_month_day(year{2025}, January, day{1}));
  EXPECT_EQ(plan.vouchers_added, 2u);
  EXPECT_EQ(plan.accounts_added, 4u);
  ASSERT_EQ(plan.books.years.size(), 1u);
  EXPECT_EQ(plan.books.years[0].vouchers.size(), 2u);
  EXPECT_EQ(plan.books.years[0].reported_opening.at("1930"), 100000);
  EXPECT_EQ(plan.books.chart_type, "EUBAS97");
}

TEST(Import, OtherOrganisationOrNameIsAnError) {
  auto const plan = plan_import({}, {"Other AB", "556999-9999"}, parse_sie(file_2025()));
  EXPECT_TRUE(has(plan.errors, "The file is for organisation number 556000-0001, but the project Other AB has "
                               "556999-9999"));
  EXPECT_TRUE(has(plan.errors, "The file's company name \"Test AB\" differs from the project's name \"Other AB\""));
  EXPECT_FALSE(plan.identity_changed);
}

TEST(Import, FileErrorsAreIncluded) {
  auto const plan = plan_import({}, fresh, parse_sie(sie_file(2025, "", "#VER A 1 20250101 \"\"\n{\n#TRANS 1930 {} 1\n}\n")));
  EXPECT_TRUE(has(plan.errors, "line 16: voucher A 1 does not balance: its amounts add up to 1.00"));
}

TEST(Import, IdenticalVouchersAreSkipped) {
  auto const books = imported({file_2025()});
  auto const plan = plan_import(books, known, parse_sie(file_2025()));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_EQ(plan.vouchers_added, 0u);
  EXPECT_EQ(plan.vouchers_known, 2u);
  EXPECT_FALSE(plan.identity_changed);
  EXPECT_EQ(plan.books.years[0].vouchers, books.years[0].vouchers);
}

namespace {

// A 1 as cratchit writes it (each row dated, the bank row on its own day when given) and as Fortnox does (no row
// dates, quantity 0)
std::string sale_as_cratchit(std::string_view bank_date = "20250201") {
  return sie_file(2025, "",
                  std::format("#VER A 1 20250201 \"Sale\" 20250205\r\n{{\r\n#TRANS 1930 {{}} 500.00 {} \"Sale\"\r\n"
                              "#TRANS 3010 {{}} -500.00 20250201 \"\"\r\n}}\r\n",
                              bank_date));
}

std::string sale_as_fortnox(std::string_view voucher_date = "20250201") {
  return sie_file(2025, "",
                  std::format("#VER A 1 {} \"Sale\" 20250205\r\n{{\r\n#TRANS 1930 {{}} 500.00 \"\" \"Sale\" 0\r\n"
                              "#TRANS 3010 {{}} -500.00 \"\" \"\" 0\r\n}}\r\n",
                              voucher_date));
}

} // namespace

TEST(Import, AVoucherWrittenAnotherWayIsTheSame) {
  auto const books = imported({sale_as_cratchit()});
  auto const plan = plan_import(books, known, parse_sie(sale_as_fortnox()));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_FALSE(plan.reverts());
  EXPECT_EQ(plan.vouchers_known, 1u);
  EXPECT_TRUE(cratchit::books::nothing_to_import(books, plan));
  EXPECT_EQ(plan.books.years[0].vouchers, books.years[0].vouchers); // the books keep their own way
  EXPECT_TRUE(std::ranges::none_of(plan.notes, [](std::string const& note) { return note.starts_with("Voucher"); }))
      << all(plan.notes);
  // The other way round too
  auto const theirs = imported({sale_as_fortnox()});
  EXPECT_TRUE(cratchit::books::nothing_to_import(theirs, plan_import(theirs, known, parse_sie(sale_as_cratchit()))));
  // A quantity is a difference
  auto quantity = sale_as_fortnox();
  quantity.replace(quantity.find("\"Sale\" 0"), 8, "\"Sale\" 2");
  EXPECT_TRUE(plan_import(books, known, parse_sie(quantity)).reverts());
}

TEST(Import, ARowDateTheFileLacksIsKept) {
  // The bank row on 2025-02-03, the day the money came; the voucher on 2025-02-01
  auto const books = imported({sale_as_cratchit("20250203")});
  auto const plan = plan_import(books, known, parse_sie(sale_as_fortnox()));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_FALSE(plan.reverts());
  EXPECT_TRUE(cratchit::books::nothing_to_import(books, plan));
  EXPECT_TRUE(has(plan.notes, "Voucher A 1: kept 1 row date the file lacks")) << all(plan.notes);
  EXPECT_EQ(plan.books.years[0].vouchers, books.years[0].vouchers);
  // With history added in the file, the changed voucher keeps the date
  auto added = sale_as_fortnox();
  added.replace(added.find("}\r\n", added.find("#VER")), 3,
                "#RTRANS 6570 {} 1 \"\" \"\" 0 \"KO\"\r\n#TRANS 6570 {} 1 \"\" \"\" 0\r\n"
                "#RTRANS 1930 {} -1 \"\" \"\" 0 \"KO\"\r\n#TRANS 1930 {} -1 \"\" \"\" 0\r\n}\r\n");
  auto const changed = plan_import(books, known, parse_sie(added));
  EXPECT_TRUE(changed.errors.empty()) << all(changed.errors);
  EXPECT_EQ(changed.vouchers_changed, 1u);
  EXPECT_EQ(changed.books.years[0].vouchers[0].postings[0].date, 2025y / February / 3);
  // A date the file gives wins
  auto other = sale_as_fortnox();
  other.replace(other.find("500.00 \"\" \"Sale\""), 16, "500.00 20250204 \"Sale\"");
  EXPECT_TRUE(plan_import(books, known, parse_sie(other)).reverts());
  // The voucher date edited in the file: the bank row keeps its own day, the row dated as the voucher follows
  auto const edited = plan_import(books, known, parse_sie(sale_as_fortnox("20250202")));
  EXPECT_TRUE(edited.errors.empty()) << all(edited.errors);
  EXPECT_EQ(edited.vouchers_changed, 1u);
  auto const& rows = edited.books.years[0].vouchers[0].postings;
  EXPECT_EQ(rows[0].date, 2025y / February / 3);
  EXPECT_FALSE(rows[1].date.has_value());
  // Renamed: the file numbered it A 7, the bank row keeps its day
  auto renamed = sale_as_fortnox();
  renamed.replace(renamed.find("#VER A 1 "), 9, "#VER A 7 ");
  auto const moved = plan_import(books, known, parse_sie(renamed));
  EXPECT_EQ(moved.renamed, std::vector<std::string>{"A 1 as A 7"});
  EXPECT_EQ(moved.books.years[0].vouchers[0].postings[0].date, 2025y / February / 3);
}

TEST(Import, AnImportedVoucherCarriesItsDebitTotalAsAmount) {
  auto const books = imported({sie_file(2025, "", "#VER A 1 20250201 \"Sale\" 20250205\n{\n#TRANS 1930 {} 500.00\n"
                                                  "#TRANS 3010 {} -400.00\n#TRANS 2099 {} -100.00\n}\n")});
  ASSERT_EQ(books.years.at(0).vouchers.size(), 1u);
  EXPECT_EQ(books.years[0].vouchers[0].event.heading, "Sale");
  EXPECT_EQ(books.years[0].vouchers[0].event.amount, 50000);
}

TEST(Import, NewVouchersOfAKnownYearAreAdded) {
  auto const books = imported({sie_file(2025, "", "#VER A 1 20250201 \"Sale\" 20250205\n{\n#TRANS 1930 {} 500.00\n"
                                                  "#TRANS 3010 {} -500.00\n}\n")});
  auto const plan = plan_import(books, known, parse_sie(file_2025()));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_EQ(plan.vouchers_added, 1u);
  EXPECT_EQ(plan.vouchers_known, 1u);
  EXPECT_EQ(plan.books.years[0].vouchers.size(), 2u);
}

TEST(Import, AnotherRegistrationDateReplacesTheVoucher) {
  auto const books = imported({file_2025()});
  auto changed = file_2025();
  changed.replace(changed.find("\"Sale\" 20250205"), 15, "\"Sale\" 20250206");
  auto const plan = plan_import(books, known, parse_sie(changed));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_EQ(plan.replaced, std::vector<std::string>{"A 1"}); // not renamed as itself
  EXPECT_TRUE(plan.renamed.empty());
  EXPECT_EQ(plan.books.years[0].vouchers[0].registered, 2025y / February / 6);
}

// The 2025 file with voucher A 1 registered a day later, which the history rule refuses
std::string registered_later() {
  auto file = file_2025();
  file.replace(file.find("\"Sale\" 20250205"), 15, "\"Sale\" 20250206");
  return file;
}

cratchit::books::VoucherKey const a1{2025y / January / 1, "A", "1"};

TEST(Import, AnImportedVoucherThatDiffersCanBeForced) {
  auto const books = imported({file_2025()});
  // Without force the file replaces it, no error to force
  auto const replaced = plan_import(books, known, parse_sie(registered_later()), {{a1}, false});
  EXPECT_TRUE(replaced.errors.empty()) << all(replaced.errors);
  EXPECT_EQ(replaced.replaced, std::vector<std::string>{"A 1"});
  EXPECT_EQ(replaced.forceable, 0u);
  EXPECT_FALSE(replaced.can_force());
  auto const forced = plan_import(books, known, parse_sie(registered_later()), {{a1}, true});
  EXPECT_TRUE(forced.errors.empty()) << all(forced.errors);
  EXPECT_EQ(forced.vouchers_forced, 1u);
  EXPECT_FALSE(forced.reverts());
  EXPECT_TRUE(has(forced.notes, "Voucher A 1 is forced: the file's replaces the one in the books (the registration "
                                "date differs)"))
      << all(forced.notes);
  ASSERT_EQ(forced.forced.size(), 1u);
  EXPECT_EQ(forced.forced[0], "A 1 in fiscal year 2025-01-01 - 2025-12-31: in the books 2025-02-01 \"Sale\" registered "
                              "2025-02-05: row 1930 500.00, row 3010 -500.00; in the file 2025-02-01 \"Sale\" "
                              "registered 2025-02-06: row 1930 500.00, row 3010 -500.00");
  EXPECT_EQ(forced.books.years[0].vouchers[0].registered, 2025y / February / 6);
  EXPECT_FALSE(cratchit::books::nothing_to_import(books, forced));
}

TEST(Import, NotForcedInAClosedYearOrOverAnEnteredVoucher) {
  auto books = imported({file_2025()});
  // Entered in cratchit: not forced, but replaced as the file is the truth
  auto const entered = plan_import(books, known, parse_sie(registered_later()), {{}, true});
  EXPECT_TRUE(entered.errors.empty()) << all(entered.errors);
  EXPECT_EQ(entered.vouchers_forced, 0u);
  EXPECT_EQ(entered.replaced, std::vector<std::string>{"A 1"});
  books.years[0].closed = true;
  auto const closed = plan_import(books, known, parse_sie(registered_later()), {{a1}, true});
  EXPECT_EQ(closed.errors, std::vector<std::string>{"The file would change 1 voucher in the books against the history "
                                                    "rule, but fiscal year 2025-01-01 - 2025-12-31 is closed. Open the "
                                                    "year to make the file the truth"});
  EXPECT_EQ(closed.vouchers_forced, 0u);
  EXPECT_FALSE(closed.can_force());
}

TEST(Import, NoForceWithOtherErrors) {
  auto const books = imported({file_2025()});
  auto const file = registered_later() + "#VER A 3 20250401 \"Odd\"\r\n{\r\n#TRANS 1930 {} 1\r\n}\r\n";
  auto const plan = plan_import(books, known, parse_sie(file), {{a1}, false});
  EXPECT_EQ(plan.forceable, 0u);
  EXPECT_FALSE(plan.errors.empty());
  EXPECT_FALSE(plan.can_force());
}

namespace {

// A voucher entered in cratchit, registered on the given day
cratchit::books::Voucher entered(std::string series, std::string number, year_month_day date, std::string heading,
                                 std::vector<std::pair<std::string, cratchit::Amount>> const& rows,
                                 year_month_day registered) {
  cratchit::books::Voucher voucher{std::move(series), std::move(number), {std::move(heading), 0, date}, registered,
                                   "", {}};
  for (auto const& [account, amount] : rows) {
    voucher.postings.push_back({cratchit::books::Posting::Kind::normal, account, {}, amount, {}, "", "", "", ""});
    voucher.event.amount += amount > 0 ? amount : 0;
  }
  return voucher;
}

std::string const lunch_a3 = "#VER A 3 20250401 \"Lunch at work\" 20261030\r\n{\r\n#TRANS 6570 {} 20\r\n"
                             "#TRANS 1930 {} -20\r\n}\r\n";

} // namespace

TEST(Import, TheSameVoucherUnderAnotherNameIsRenamed) {
  auto books = imported({file_2025()});
  books.years[0].vouchers.push_back(
      entered("D", "1", 2025y / April / 1, "Lunch", {{"6570", 2000}, {"1930", -2000}}, 2026y / October / 25));
  auto const plan = plan_import(books, known, parse_sie(sie_file(2025, balanced_balances(), sale_and_cost(2025) + lunch_a3)));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_EQ(plan.renamed, std::vector<std::string>{"D 1 as A 3"});
  EXPECT_TRUE(plan.demoted.empty());
  EXPECT_EQ(plan.vouchers_added, 0u);
  EXPECT_EQ(plan.vouchers_known, 2u);
  auto const& vouchers = plan.books.years[0].vouchers;
  ASSERT_EQ(vouchers.size(), 3u);
  EXPECT_EQ(vouchers[2].series + vouchers[2].number, "A3");
  EXPECT_EQ(vouchers[2].event.heading, "Lunch at work");
  // The same text first
  auto const two = plan_import(
      books, known,
      parse_sie(sie_file(2025, balanced_balances(),
                         sale_and_cost(2025) + lunch_a3 +
                             "#VER A 4 20250401 \"Lunch\"\r\n{\r\n#TRANS 1930 {} -20\r\n#TRANS 6570 {} 20\r\n}\r\n")));
  EXPECT_EQ(two.renamed, std::vector<std::string>{"D 1 as A 4"});
  EXPECT_EQ(two.vouchers_added, 1u);
}

TEST(Import, ACorrectionAndWhatItCorrectsAreDroppedTheRestWaits) {
  auto books = imported({file_2025()});
  auto& vouchers = books.years[0].vouchers;
  vouchers.push_back(entered("A", "3", 2025y / April / 1, "Refund", {{"1930", 3500}, {"6570", -3500}},
                             2026y / October / 25));
  vouchers.push_back(entered("D", "1", 2025y / April / 2, "Lunch", {{"6570", 2000}, {"1930", -2000}},
                             2026y / October / 1));
  vouchers.push_back(entered("A", "4", 2025y / April / 1, "Rättelse av A 3: Refund", {{"1930", -3500}, {"6570", 3500}},
                             2026y / October / 27));
  auto const plan = plan_import(books, known, parse_sie(file_2025())); // #GEN 2026-10-19
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_EQ(plan.dropped, (std::vector<std::string>{"A 3", "A 4"}));
  EXPECT_EQ(plan.demoted, std::vector<std::string>{"D 1"});
  // A credit note on another day is no correction: both wait
  auto credited = books;
  credited.years[0].vouchers[4].event.date = 2025y / April / 9;
  EXPECT_TRUE(plan_import(credited, known, parse_sie(file_2025())).dropped.empty());
  EXPECT_EQ(plan.registered_after_file, 2u); // D 1 was registered before the file was made
  EXPECT_EQ(plan.reverted.size(), 3u);
  EXPECT_EQ(plan.books.years[0].vouchers.size(), 2u);
  ASSERT_EQ(plan.books.years[0].drafts.size(), 1u);
  EXPECT_EQ(plan.books.years[0].drafts[0].noted, 2026y / October / 1);
  // Brought back by a file, D 1 takes its draft off the list; another draft stays
  auto reverted = plan.books;
  reverted.years[0].place_draft({{"Coffee", 2000, 2025y / April / 2}, 2025y / April / 2,
                                   {{cratchit::books::Posting::Kind::normal, "6570", {}, 2000, 2025y / April / 2, "",
                                     "", "", ""}}});
  auto const back = plan_import(
      reverted, known,
      parse_sie(sie_file(2025, balanced_balances(),
                         sale_and_cost(2025) + "#VER D 1 20250402 \"Lunch\"\r\n{\r\n#TRANS 6570 {} 20\r\n"
                                               "#TRANS 1930 {} -20\r\n}\r\n")));
  EXPECT_TRUE(back.errors.empty()) << all(back.errors);
  EXPECT_FALSE(back.reverts());
  EXPECT_EQ(back.drafts_booked, std::vector<std::string>{"2025-04-02 \"Lunch\" as D 1"});
  ASSERT_EQ(back.books.years[0].drafts.size(), 1u);
  EXPECT_EQ(back.books.years[0].drafts[0].event.heading, "Coffee");
  // Not in a closed year
  books.years[0].closed = true;
  auto const closed = plan_import(books, known, parse_sie(file_2025()));
  EXPECT_EQ(closed.errors.size(), 1u) << all(closed.errors);
}

TEST(Import, AnEditedTextOrDateChangesAVoucher) {
  auto const books = imported({file_2025()});
  auto text = file_2025();
  text.replace(text.find("\"Bank cost\""), 11, "\"Bank fee\"");
  auto const by_text = plan_import(books, known, parse_sie(text));
  EXPECT_TRUE(by_text.errors.empty()) << all(by_text.errors);
  EXPECT_TRUE(has(by_text.notes, "Voucher A 2 changes: text \"Bank cost\" to \"Bank fee\"")) << all(by_text.notes);
  EXPECT_EQ(by_text.vouchers_changed, 1u);
  EXPECT_EQ(by_text.books.years[0].vouchers[1].event.heading, "Bank fee");
  auto date = file_2025();
  date.replace(date.find("A 2 20250301"), 12, "A 2 20250303");
  auto const by_date = plan_import(books, known, parse_sie(date));
  EXPECT_TRUE(by_date.errors.empty()) << all(by_date.errors);
  EXPECT_TRUE(has(by_date.notes, "Voucher A 2 changes: date 2025-03-01 to 2025-03-03")) << all(by_date.notes);
  EXPECT_EQ(by_date.books.years[0].vouchers[1].event.date, 2025y / March / 3);
}

// The 2025 file with the given rows (CRLF lines) in voucher A 2 instead of 6570 10 and 1930 -10
std::string with_a2_rows(std::string_view rows) {
  auto file = file_2025();
  std::string_view const original = "#TRANS 6570 {} 10\r\n#TRANS 1930 {} -10\r\n";
  file.replace(file.find(original), original.size(), rows);
  return file;
}

// A 2 with both rows struck and new ones added for 12, as Fortnox writes a changed voucher
std::string const struck_and_added = "#BTRANS 6570 {} 10 \"\" \"\" \"\" \"Kjell\"\r\n"
                                     "#BTRANS 1930 {} -10 \"\" \"\" \"\" \"Kjell\"\r\n"
                                     "#RTRANS 6570 {} 12 \"\" \"\" \"\" \"Kjell\"\r\n#TRANS 6570 {} 12\r\n"
                                     "#RTRANS 1930 {} -12 \"\" \"\" \"\" \"Kjell\"\r\n#TRANS 1930 {} -12\r\n";

TEST(Import, StruckAndAddedRowsChangeAVoucher) {
  auto const books = imported({file_2025()});
  auto const plan = plan_import(books, known, parse_sie(with_a2_rows(struck_and_added)));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_TRUE(has(plan.notes, "Voucher A 2 changes: 2 rows struck, 2 added (signed Kjell)")) << all(plan.notes);
  EXPECT_EQ(plan.vouchers_changed, 1u);
  EXPECT_EQ(plan.vouchers_known, 1u);
  auto const& a2 = plan.books.years[0].vouchers[1];
  ASSERT_EQ(a2.postings.size(), 4u);
  EXPECT_EQ(a2.postings[0].kind, cratchit::books::Posting::Kind::removed);
  EXPECT_EQ(a2.postings[2].kind, cratchit::books::Posting::Kind::added);
  EXPECT_EQ(a2.event.amount, 1200);
  // The same file again: nothing to import
  auto const again = plan_import(plan.books, known, parse_sie(with_a2_rows(struck_and_added)));
  EXPECT_TRUE(again.errors.empty()) << all(again.errors);
  EXPECT_EQ(again.vouchers_changed, 0u);
  EXPECT_TRUE(cratchit::books::nothing_to_import(plan.books, again));
}

TEST(Import, AnEditedDateWithStruckAndAddedRows) {
  auto const books = imported({file_2025()});
  auto file = with_a2_rows(struck_and_added);
  file.replace(file.find("A 2 20250301"), 12, "A 2 20250303");
  auto const plan = plan_import(books, known, parse_sie(file));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_TRUE(has(plan.notes, "Voucher A 2 changes: date 2025-03-01 to 2025-03-03, 2 rows struck, 2 added (signed "
                              "Kjell)"))
      << all(plan.notes);
}

TEST(Import, AnAddedRowMayBeStruckLater) {
  auto const books = imported({file_2025(), with_a2_rows(struck_and_added)});
  auto const plan = plan_import(books, known,
                                parse_sie(with_a2_rows("#BTRANS 6570 {} 10 \"\" \"\" \"\" \"Kjell\"\r\n"
                                                       "#BTRANS 1930 {} -10 \"\" \"\" \"\" \"Kjell\"\r\n"
                                                       "#BTRANS 6570 {} 12 \"\" \"\" \"\" \"Anna\"\r\n"
                                                       "#BTRANS 1930 {} -12 \"\" \"\" \"\" \"Anna\"\r\n"
                                                       "#RTRANS 6570 {} 11 \"\" \"\" \"\" \"Anna\"\r\n"
                                                       "#TRANS 6570 {} 11\r\n"
                                                       "#RTRANS 1930 {} -11 \"\" \"\" \"\" \"Anna\"\r\n"
                                                       "#TRANS 1930 {} -11\r\n")));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_TRUE(has(plan.notes, "Voucher A 2 changes: 2 rows struck, 2 added (signed Anna)")) << all(plan.notes);
  EXPECT_EQ(plan.books.years[0].vouchers[1].event.amount, 1100);
}

TEST(Import, OtherChangesToAVoucherReplaceIt) {
  auto const books = imported({file_2025()});
  auto const replaces = [](ImportPlan const& plan) {
    EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
    EXPECT_EQ(plan.replaced, std::vector<std::string>{"A 2"});
    EXPECT_EQ(plan.vouchers_changed, 0u);
    EXPECT_EQ(plan.vouchers_added, 0u);
    EXPECT_EQ(plan.books.years[0].vouchers.size(), 2u);
  };
  auto const changed_row = plan_import(books, known, parse_sie(with_a2_rows("#TRANS 6570 {} 12\r\n#TRANS 1930 {} -12\r\n")));
  replaces(changed_row);
  EXPECT_EQ(changed_row.books.years[0].vouchers[1].postings[0].amount, 1200);
  replaces(plan_import(books, known,
                       parse_sie(with_a2_rows("#TRANS 6570 {} 10\r\n#TRANS 1930 {} -10\r\n"
                                              "#TRANS 6570 {} 1\r\n#TRANS 1930 {} -1\r\n"))));
  // A struck row that comes back, or a row left out
  auto const changed = imported({file_2025(), with_a2_rows(struck_and_added)});
  replaces(plan_import(changed, known,
                       parse_sie(with_a2_rows("#TRANS 6570 {} 10\r\n#TRANS 1930 {} -10\r\n"
                                              "#RTRANS 6570 {} 12 \"\" \"\" \"\" \"Kjell\"\r\n"
                                              "#TRANS 6570 {} 12\r\n"
                                              "#RTRANS 1930 {} -12 \"\" \"\" \"\" \"Kjell\"\r\n"
                                              "#TRANS 1930 {} -12\r\n"))));
  replaces(plan_import(changed, known, parse_sie(file_2025())));
}

TEST(Import, AChangeInAClosedYearIsAnError) {
  auto books = imported({file_2025()});
  books.years[0].closed = true;
  auto const plan = plan_import(books, known, parse_sie(with_a2_rows(struck_and_added)));
  EXPECT_TRUE(has(plan.errors, "Voucher A 2 in the file changes the one in the books (2 rows struck, 2 added (signed "
                               "Kjell)), but fiscal year 2025-01-01 - 2025-12-31 is closed. Open the year to import "
                               "the change"))
      << all(plan.errors);
  EXPECT_EQ(plan.vouchers_changed, 0u);
  // Also an edited text
  auto text = file_2025();
  text.replace(text.find("\"Bank cost\""), 11, "\"Bank fee\"");
  auto const by_text = plan_import(books, known, parse_sie(text));
  EXPECT_TRUE(has(by_text.errors, "Voucher A 2 in the file changes the one in the books (text \"Bank cost\" to \"Bank "
                                  "fee\"), but fiscal year 2025-01-01 - 2025-12-31 is closed. Open the year to import "
                                  "the change"))
      << all(by_text.errors);
}

TEST(Import, AVoucherMissingFromTheFileWaits) {
  auto const books = imported({file_2025()});
  auto const plan = plan_import(books, known,
                                parse_sie(sie_file(2025, "", "#VER A 1 20250201 \"Sale\" 20250205\n{\n#TRANS 1930 {} "
                                                             "500.00\n#TRANS 3010 {} -500.00\n}\n")));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_TRUE(plan.reverts());
  EXPECT_EQ(plan.demoted, std::vector<std::string>{"A 2"});
  EXPECT_EQ(plan.reverted, std::vector<std::string>{"Demoted to a draft A 2 in fiscal year 2025-01-01 - "
                                                    "2025-12-31: 2025-03-01 \"Bank cost\": row 6570 10.00, row 1930 "
                                                    "-10.00"});
  auto const& year = plan.books.years[0];
  ASSERT_EQ(year.vouchers.size(), 1u);
  EXPECT_EQ(year.vouchers[0].series + year.vouchers[0].number, "A1");
  ASSERT_EQ(year.drafts.size(), 1u);
  EXPECT_EQ(year.drafts[0].event.heading, "Bank cost");
  EXPECT_EQ(year.drafts[0].event.amount, 1000);
  EXPECT_EQ(year.drafts[0].noted, 2025y / March / 1); // no registration date: the voucher's date
  ASSERT_EQ(year.drafts[0].postings.size(), 2u);
  EXPECT_EQ(year.drafts[0].postings[0].account, "6570");
  EXPECT_EQ(year.drafts[0].postings[0].date, 2025y / March / 1);
  EXPECT_FALSE(cratchit::books::nothing_to_import(books, plan));
}

TEST(Import, OverlappingOrDifferentlyEndingYearIsAnError) {
  auto const books = imported({file_2025()});
  auto shifted = file_2025();
  shifted.replace(shifted.find("#RAR 0 20250101 20251231"), 24, "#RAR 0 20250701 20260630");
  auto const overlap = plan_import(books, known, parse_sie(shifted));
  EXPECT_TRUE(has(overlap.errors, "The file's fiscal year 2025-07-01 - 2026-06-30 overlaps the year 2025-01-01 - "
                                  "2025-12-31 in the books"))
      << all(overlap.errors);
  auto ending = file_2025();
  ending.replace(ending.find("#RAR 0 20250101 20251231"), 24, "#RAR 0 20250101 20251130");
  auto const other_end = plan_import(books, known, parse_sie(ending));
  EXPECT_TRUE(has(other_end.errors, "The file's fiscal year 2025-01-01 - 2025-11-30 ends otherwise than the one in "
                                    "the books (2025-01-01 - 2025-12-31)"))
      << all(other_end.errors);
}

TEST(Import, YearsInEitherOrderGiveTheSameBooks) {
  auto const forward = imported({file_2025(), file_2026()});
  auto const backward = imported({file_2026(), file_2025()});
  ASSERT_EQ(forward.years.size(), 2u);
  ASSERT_EQ(backward.years.size(), 2u);
  EXPECT_EQ(forward.years[0].start, backward.years[0].start);
  EXPECT_EQ(forward.years[1].vouchers, backward.years[1].vouchers);
  EXPECT_EQ(forward.accounts, backward.accounts);
  auto const a = cratchit::books::balances(forward);
  auto const b = cratchit::books::balances(backward);
  EXPECT_EQ(a[1].closing, b[1].closing);
  EXPECT_EQ(a[1].closing.at("1930"), 159000);
}

TEST(Import, ConsistentNextYearHasNoBalanceNotes) {
  auto const books = imported({file_2025()});
  auto const plan = plan_import(books, known, parse_sie(file_2026()));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_TRUE(plan.notes.empty()) << all(plan.notes);
}

TEST(Import, OtherOpeningBalanceIsNotedAndCratchitsOwnIsUsed) {
  auto const books = imported({file_2025()});
  // The file opens 1930 with 1500, cratchit's 2025 closing is 1490: 10 off, as voucher A 2 (bank cost 10)
  auto const plan = plan_import(books, known, parse_sie(file_2026("1500.00")));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_TRUE(has(plan.notes, "Opening balance 2026-01-01, 1930 Bank: the file says 1500.00, cratchit carries 1490.00 "
                              "from its closing balance 2025-12-31 (difference -10.00) and uses its own. Possibly from "
                              "voucher A 2 2025-03-01 \"Bank cost\""))
      << all(plan.notes);
  auto const after = cratchit::books::balances(plan.books);
  EXPECT_EQ(after[1].opening.at("1930"), 149000);
}

TEST(Import, EarlierYearInTheFileIsCheckedAgainstTheBooks) {
  auto const books = imported({file_2025()});
  auto file = file_2026();
  file.replace(file.find("#UB -1 1930 1490.00"), 19, "#UB -1 1930 1480.00");
  file.replace(file.find("#RES -1 6570 10"), 15, "#RES -1 6570 20");
  auto const plan = plan_import(books, known, parse_sie(file));
  EXPECT_TRUE(has(plan.notes, "Closing balance 2025-12-31, 1930 Bank: the file says 1480.00, cratchit's books give "
                              "1490.00 (difference 10.00). Possibly from voucher A 2 2025-03-01 \"Bank cost\""))
      << all(plan.notes);
  EXPECT_TRUE(has(plan.notes, "Result 2025-01-01 - 2025-12-31, 6570 Bank costs: the file says 20.00, cratchit's books "
                              "give 10.00 (difference -10.00). Possibly from voucher A 2 2025-03-01 \"Bank cost\""))
      << all(plan.notes);
}

TEST(Import, AnEarlierYearChangesTheNextYearsOpeningBalances) {
  auto const books = imported({file_2026("1500.00")});
  auto const plan = plan_import(books, known, parse_sie(file_2025()));
  EXPECT_TRUE(plan.errors.empty()) << all(plan.errors);
  EXPECT_TRUE(has(plan.notes, "Opening balance 2026-01-01, 1930 Bank: changes from 1500.00 to 1490.00, carried from "
                              "the closing balance 2025-12-31"))
      << all(plan.notes);
}

TEST(Import, AccountChangesOnlyFromTheLatestYear) {
  auto renamed = [](std::string file) {
    file.replace(file.find("#KONTO 1930 \"Bank\""), 18, "#KONTO 1930 \"Bank account\"");
    return file;
  };
  // The 2025 file renames 1930 but 2026 is in the books: the name stays
  auto const newer = imported({file_2026()});
  auto const older = plan_import(newer, known, parse_sie(renamed(file_2025())));
  EXPECT_EQ(older.books.account("1930")->name, "Bank");
  EXPECT_EQ(older.accounts_changed, 0u);
  // The 2026 file renames it with 2025 in the books: the name changes, noted
  auto const latest = plan_import(imported({file_2025()}), known, parse_sie(renamed(file_2026())));
  EXPECT_EQ(latest.books.account("1930")->name, "Bank account");
  EXPECT_EQ(latest.accounts_changed, 1u);
  EXPECT_TRUE(has(latest.notes, "Account 1930: name \"Bank\" becomes \"Bank account\"")) << all(latest.notes);
}
