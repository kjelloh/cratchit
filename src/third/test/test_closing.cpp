#include "core/closing.hpp"
#include "core/sie/document.hpp"
#include "core/sie/file.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

using cratchit::Balances;
using cratchit::check_closing;
using namespace std::chrono;

namespace {

constexpr cratchit::Date balance_day = 2023y / April / 30;

// The one note, or empty
std::string note_of(Balances const& balances) {
  auto const notes = check_closing(balances, balance_day);
  EXPECT_LE(notes.size(), 1u);
  return notes.empty() ? "" : notes.front();
}

} // namespace

TEST(Closing, ClosedBooksHaveNoNotes) {
  EXPECT_TRUE(check_closing({{"1650", 133146}, {"1630", 63000}, {"2650", -5000}, {"2510", -62500}, {"2999", 0}},
                            balance_day)
                  .empty());
}

TEST(Closing, VatNettingToAReceivable) {
  // Unreported ingående moms is larger than utgående moms
  EXPECT_EQ(note_of({{"2611", -10000}, {"2641", 133146}}),
            "Moms (2610-2659) nets to a receivable 1231.46 at 2023-04-30; BAS: move it to 1650 Momsfordran");
  EXPECT_EQ(note_of({{"2650", 42800}}),
            "Moms (2610-2659) nets to a receivable 428.00 at 2023-04-30; BAS: move it to 1650 Momsfordran");
  EXPECT_EQ(note_of({{"2611", -20000}, {"2641", 13314}}), "");
  // Punktskatter and OSS are not part of it
  EXPECT_EQ(note_of({{"2660", 100}}), "");
}

TEST(Closing, TaxNettingToAReceivable) {
  EXPECT_NE(note_of({{"2510", 30000}, {"2512", -10000}}).find("1640 Skattefordringar"), std::string::npos);
  EXPECT_EQ(note_of({{"2510", -30000}}), "");
}

TEST(Closing, TaxAccountWithCreditBalance) {
  EXPECT_EQ(note_of({{"1630", -2500}}), "1630 Skattekonto has a credit balance -25.00 at 2023-04-30; BAS: move it to 2850");
  EXPECT_EQ(note_of({{"1630", 2500}}), "");
}

TEST(Closing, RedovisningsmedelNotHeldForOthers) {
  EXPECT_NE(note_of({{"1999", 15675}}).find("Redovisningsmedel (1990-1999) 156.75"), std::string::npos);
  EXPECT_EQ(note_of({{"1990", 50000}, {"2830", -50000}}), "");
}

TEST(Closing, SuspenseAccountNotZero) {
  EXPECT_EQ(note_of({{"2999", -100}}), "2999 OBS-konto is -1.00 at 2023-04-30; BAS: it must be zero at year end");
}

namespace {

// The notes for years 0 and -1 of one of the session's SIE files (git ignored), or nothing when missing
std::optional<std::vector<std::string>> notes_of_file(char const* name) {
  auto const sie = std::filesystem::path(CRATCHIT_INK2_SIE_FOLDER) / name;
  if (!std::filesystem::exists(sie)) {
    return std::nullopt;
  }
  auto const lexed = cratchit::sie::lex_file(sie);
  EXPECT_TRUE(lexed);
  auto const document = cratchit::sie::parse_document(lexed->lines).document;
  std::vector<std::string> notes;
  for (int index = 0; index >= -1; --index) {
    for (auto const& note : check_closing(cratchit::year_balances(document, index), document.year(index)->end)) {
      notes.push_back(note.substr(0, note.find(';')));
    }
  }
  return notes;
}

} // namespace

TEST(Closing, FiledYearsLackTheVatClosingEntry) {
  // Deduced from the vouchers: open VAT and unreported VAT parked on 1999 "OBS" at each year end
  auto const notes = notes_of_file("TheITfiedAB20230503_152345.se");
  if (!notes) {
    GTEST_SKIP() << "No SIE file";
  }
  EXPECT_EQ(*notes, (std::vector<std::string>{
                        "Moms (2610-2659) nets to a receivable 1331.46 at 2023-04-30",
                        "Redovisningsmedel (1990-1999) 156.75 at 2023-04-30 is not matched by Avräkning för annans "
                        "räkning (2830-2839) 0.00",
                        "Moms (2610-2659) nets to a receivable 428.00 at 2022-04-30",
                        "Redovisningsmedel (1990-1999) 161.25 at 2022-04-30 is not matched by Avräkning för annans "
                        "räkning (2830-2839) 0.00",
                    }));
  // 23/24 also closed with last year's result left on 2098 (corrected in the books after this export)
  auto const notes_2324 = *notes_of_file("TheITfiedAB20240727_213539.se");
  EXPECT_EQ(notes_2324.size(), 4u);
  EXPECT_NE(std::ranges::find(notes_2324, "2098 Vinst eller förlust från föregående år is 278845.50 at 2024-04-30"),
            notes_2324.end());
  EXPECT_EQ(*notes_of_file("TheITfiedAB20250817_172409.se"),
            (std::vector<std::string>{
                "Moms (2610-2659) nets to a receivable 67.60 at 2025-04-30",
                "Redovisningsmedel (1990-1999) 336.00 at 2025-04-30 is not matched by Avräkning för annans räkning "
                "(2830-2839) 0.00",
                "Moms (2610-2659) nets to a receivable 156.75 at 2024-04-30",
            }));
}

namespace {

// Opening: 100 kr in the bank against share capital. In the year: 30 kr of costs paid from the bank.
cratchit::sie::Document unclosed_books() {
  cratchit::sie::Document document;
  document.years = {{0, 2025y / May / 1, 2026y / April / 30}};
  document.opening = {{0, "1930", 10000, ""}, {0, "2081", -10000, ""}};
  document.closing = {{0, "1930", 7000, ""}, {0, "2081", -10000, ""}};
  document.results = {{0, "6110", 3000, ""}};
  return document;
}

} // namespace

TEST(Closing, UnbookedResultIsFound) {
  // A loss of 30 kr is a debit of 30 kr on the result accounts, and the balance accounts lack it
  EXPECT_EQ(cratchit::unbooked_result(unclosed_books()), 3000);
}

TEST(Closing, ClosedBooksHaveNoUnbookedResult) {
  auto document = unclosed_books();
  document.closing.push_back({0, "2099", 3000, ""});
  document.results.push_back({0, "8999", -3000, ""});
  EXPECT_FALSE(cratchit::unbooked_result(document));
}

TEST(Closing, AnotherDifferenceIsNotAnUnbookedResult) {
  auto document = unclosed_books();
  document.closing[0].amount = 6900; // a krona missing besides the result
  EXPECT_FALSE(cratchit::unbooked_result(document));
  auto nothing = unclosed_books();
  nothing.results.clear();
  nothing.closing[0].amount = 10000;
  EXPECT_FALSE(cratchit::unbooked_result(nothing));
}

TEST(Closing, SimulatedClosingEntryBalancesTheBooks) {
  auto const document = cratchit::with_closing_entry(unclosed_books(), 3000);
  ASSERT_EQ(document.vouchers.size(), 1u);
  auto const& voucher = document.vouchers.front();
  EXPECT_EQ(voucher.series, "*");
  EXPECT_EQ(voucher.date, 2026y / April / 30);
  EXPECT_EQ(voucher.text, cratchit::simulated_closing_text);
  ASSERT_EQ(voucher.transactions.size(), 2u);
  EXPECT_EQ(voucher.transactions[0].account, "2099");
  EXPECT_EQ(voucher.transactions[0].amount, 3000);
  EXPECT_EQ(voucher.transactions[1].account, "8999");
  EXPECT_EQ(voucher.transactions[1].amount, -3000);
  EXPECT_FALSE(cratchit::unbooked_result(document));
  auto const balances = cratchit::year_balances(document, 0);
  EXPECT_EQ(balances.at("2099"), 3000);
  EXPECT_EQ(balances.at("8999"), -3000);
}

TEST(Closing, NoDepreciationIsNoted) {
  EXPECT_EQ(note_of({{"1229", -50000}}),
            "No depreciation (7800-7899) in the year ending 2023-04-30, although 1229 (accumulated depreciation) is "
            "-500.00; depreciation is a closing entry");
  EXPECT_EQ(note_of({{"1229", -50000}, {"7832", 10000}}), "");
}

TEST(Closing, LastYearsResultLeftOn2098IsNoted) {
  EXPECT_EQ(note_of({{"2098", 4670178}}),
            "2098 Vinst eller förlust från föregående år is 46701.78 at 2023-04-30; last year's result is not yet "
            "moved on as the årsstämma decided (e.g. to 2091 Balanserat resultat)");
}

TEST(Closing, UnclosedFile) {
  // The user's books for 2025-26, exported before closing (git ignored)
  auto const sie = std::filesystem::path(CRATCHIT_INK2_SIE_FOLDER) / "sie_2025-26_all.se";
  if (!std::filesystem::exists(sie)) {
    GTEST_SKIP() << "No " << sie;
  }
  auto const lexed = cratchit::sie::lex_file(sie);
  ASSERT_TRUE(lexed);
  auto const original = cratchit::sie::parse_document(lexed->lines).document;
  auto const unbooked = cratchit::unbooked_result(original);
  ASSERT_TRUE(unbooked);
  EXPECT_EQ(*unbooked, 2498949);
  auto const document = cratchit::with_closing_entry(original, *unbooked);
  EXPECT_FALSE(cratchit::unbooked_result(document));
  auto const notes = notes_of_file("sie_2025-26_all.se");
  ASSERT_TRUE(notes);
  EXPECT_NE(std::ranges::find(*notes, "No depreciation (7800-7899) in the year ending 2026-04-30, although 1229 "
                                      "(accumulated depreciation) is -278114.96"),
            notes->end());
  EXPECT_NE(std::ranges::find(*notes, "2098 Vinst eller förlust från föregående år is 46701.78 at 2026-04-30"),
            notes->end());
}
