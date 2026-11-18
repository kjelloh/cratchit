#include "core/books/books.hpp"
#include "core/event_draft.hpp"
#include "core/sie_project.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace cratchit;
using cratchit::books::Event;
using namespace std::chrono;

namespace {

Date const oct20{2026y, October, 20d};

Event event(std::string_view text) {
  auto const parsed = parse_event(text);
  EXPECT_TRUE(parsed.has_value()) << text << ": " << (parsed ? "" : parsed.error());
  EXPECT_EQ(parsed ? parsed->size() : 0u, 1u) << text;
  return parsed && !parsed->empty() ? parsed->front() : Event{};
}

std::vector<Event> readings(std::string_view text) {
  auto const parsed = parse_event(text);
  EXPECT_TRUE(parsed.has_value()) << text << ": " << (parsed ? "" : parsed.error());
  return parsed.value_or(std::vector<Event>{});
}

std::string error_of(std::string_view text) {
  auto const parsed = parse_event(text);
  EXPECT_FALSE(parsed.has_value()) << text;
  return parsed ? std::string() : parsed.error();
}

} // namespace

TEST(ParseEvent, HeadingAmountAndDateInAnyOrder) {
  EXPECT_EQ(event("Telia faktura oktober 1118,00 2026-10-20"), (Event{"Telia faktura oktober", 111800, oct20}));
  EXPECT_EQ(event("20261020 1118.5 Telia faktura"), (Event{"Telia faktura", 111850, oct20}));
  EXPECT_EQ(event("Telia 2026-10-20 faktura 1118"), (Event{"Telia faktura", 111800, oct20}));
  EXPECT_EQ(event("qliro faktura 500 2026-10-20"), (Event{"qliro faktura", 50000, oct20}));
}

TEST(ParseEvent, TheLastNumberIsTheAmount) {
  EXPECT_EQ(event("Faktura 87 Payex 500 2026-10-20"), (Event{"Faktura 87 Payex", 50000, oct20}));
  // Not groups of thousands: the last word is not three digits, or the word before is too long
  EXPECT_EQ(event("Faktura 87 50,00 2026-10-20"), (Event{"Faktura 87", 5000, oct20}));
  EXPECT_EQ(event("Faktura 8700 500 2026-10-20"), (Event{"Faktura 8700", 50000, oct20}));
  EXPECT_EQ(event("Faktura 87,5 500 2026-10-20"), (Event{"Faktura 87,5", 50000, oct20}));
}

TEST(ParseEvent, SpacedNumbersGiveOneReadingPerGrouping) {
  EXPECT_EQ(readings("Faktura 87 500 2026-10-20"), (std::vector<Event>{{"Faktura 87", 50000, oct20}, {"Faktura", 8750000, oct20}}));
  EXPECT_EQ(readings("Telia 1 118,00 2026-10-20"), (std::vector<Event>{{"Telia 1", 11800, oct20}, {"Telia", 111800, oct20}}));
  EXPECT_EQ(readings("Lön 1 234 567,50 2026-10-20"),
            (std::vector<Event>{{"Lön 1 234", 56750, oct20}, {"Lön 1", 23456750, oct20}, {"Lön", 123456750, oct20}}));
  // A reading without heading is left out
  EXPECT_EQ(readings("1 118,00 Telia 2026-10-20"), (std::vector<Event>{{"1 Telia", 11800, oct20}, {"Telia", 111800, oct20}}));
}

TEST(ParseEvent, NoBreakSpaceInsideANumberIsClear) {
  EXPECT_EQ(event("Telia 1\xC2\xA0" "118,00 2026-10-20"), (Event{"Telia", 111800, oct20}));
}

TEST(ParseEvent, TheLastDateIsTheDate) {
  EXPECT_EQ(event("Momsrapport 2026-07-01 2026-09-30 47,11"),
            (Event{"Momsrapport 2026-07-01", 4711, Date{2026y, September, 30d}}));
}

TEST(ParseEvent, Errors) {
  EXPECT_EQ(error_of("Telia faktura 2026-10-20"), "Inget belopp. Skriv det med siffror, till exempel 1118,00.");
  EXPECT_EQ(error_of("Telia 1118"), "Inget datum. Skriv datumet, till exempel 2026-10-22.");
  EXPECT_EQ(error_of("1118 2026-10-20"), "Ingen rubrik. Skriv vad som hände, till exempel Telia faktura oktober.");
  EXPECT_EQ(error_of("Telia 0,00 2026-10-20"), "Beloppet är noll.");
  EXPECT_EQ(error_of("Telia 1118,005 2026-10-20"), "1118,005 är inget belopp (högst två decimaler).");
  EXPECT_EQ(error_of(""), "Inget belopp. Skriv det med siffror, till exempel 1118,00.");
}

TEST(ParseEvent, AnAmountBelowZeroKeepsItsSign) {
  // Minus for an event the other way than usual, e.g. a credit note (beslut 2026-11-17)
  EXPECT_EQ(event("Telia kreditfaktura -1245,00 2026-10-20"), (Event{"Telia kreditfaktura", -124500, oct20}));
  // Groups of thousands after a minus are unclear as after any number
  EXPECT_EQ(readings("Utbetalning -1 118,00 2026-10-20"),
            (std::vector<Event>{{"Utbetalning -1", 11800, oct20}, {"Utbetalning", -111800, oct20}}));
}

TEST(AddEventDraft, AddsTheEventAsADraftInsideTheYear) {
  auto const folder = fs::temp_directory_path() / "cratchit_event_draft_test";
  fs::remove_all(folder);
  fs::create_directories(folder);
  std::ofstream(folder / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20260101 20261231\r\n"
         "#KONTO 1930 \"Bank\"\r\n";
  auto plan = plan_project_from_sie(folder / "test.se");
  ASSERT_TRUE(plan) << plan.error();
  ASSERT_TRUE(create_project_from_sie(*plan, "2026-11-09"));
  Date const start{2026y, January, 1d};
  Date const today{2026y, November, 17d};
  auto added = add_event_draft(folder, start, {"Telia faktura oktober", 111800, oct20}, today);
  ASSERT_TRUE(added) << added.error();
  EXPECT_EQ(*added, 0u);
  added = add_event_draft(folder, start, {"Loopia", 130875, Date{2026y, September, 28d}}, today);
  ASSERT_TRUE(added) << added.error();
  EXPECT_EQ(*added, 0u); // earlier: first
  auto const refused = add_event_draft(folder, start, {"Nästa år", 100, Date{2027y, January, 2d}}, today);
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "2027-01-02 ligger utanför räkenskapsåret 2026-01-01 – 2026-12-31");
  auto const books = books::read_books(folder);
  ASSERT_TRUE(books) << books.error();
  auto const* year = books->year_starting(start);
  ASSERT_NE(year, nullptr);
  ASSERT_EQ(year->drafts.size(), 2u);
  EXPECT_EQ(year->drafts[1].event, (Event{"Telia faktura oktober", 111800, oct20}));
  EXPECT_EQ(year->drafts[1].noted, today);
  EXPECT_TRUE(year->drafts[1].postings.empty());
  EXPECT_EQ(books::stage(year->drafts[1]), books::DraftStage::event);
  fs::remove_all(folder);
}

TEST(ForgetDraft, ForgetsTheDraftSeenOnly) {
  auto const folder = fs::temp_directory_path() / "cratchit_forget_draft_test";
  fs::remove_all(folder);
  fs::create_directories(folder);
  std::ofstream(folder / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20260101 20261231\r\n";
  auto plan = plan_project_from_sie(folder / "test.se");
  ASSERT_TRUE(plan) << plan.error();
  ASSERT_TRUE(create_project_from_sie(*plan, "2026-11-09"));
  Date const start{2026y, January, 1d};
  Date const today{2026y, November, 17d};
  ASSERT_TRUE(add_event_draft(folder, start, {"Telia", 111800, oct20}, today));
  ASSERT_TRUE(add_event_draft(folder, start, {"Loopia", 130875, Date{2026y, November, 1d}}, today));
  auto const books = books::read_books(folder);
  ASSERT_TRUE(books);
  auto const loopia = books->years.front().drafts[1];
  // Not the draft seen at that place: refused, nothing changes
  auto const refused = forget_draft(folder, start, 0, loopia);
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "Utkastet finns inte längre på sin plats i bokföringen. Öppna det igen från Utkast.");
  EXPECT_FALSE(forget_draft(folder, start, 5, loopia));
  // The one seen
  ASSERT_TRUE(forget_draft(folder, start, 1, loopia));
  auto const after = books::read_books(folder);
  ASSERT_TRUE(after);
  ASSERT_EQ(after->years.front().drafts.size(), 1u);
  EXPECT_EQ(after->years.front().drafts[0].event.heading, "Telia");
  fs::remove_all(folder);
}

TEST(DraftStage, ACreditNoteIsReadyWhenDebitAddsUpToItsAmountWithoutSign) {
  books::Draft draft{{"Telia kreditfaktura", -124500, oct20}, oct20, {}};
  auto posting = [](std::string account, Amount amount) {
    books::Posting p;
    p.account = std::move(account);
    p.amount = amount;
    return p;
  };
  draft.postings = {posting("2440", 124500), posting("6212", -99600), posting("2641", -24900)};
  EXPECT_EQ(books::stage(draft), books::DraftStage::ready);
  draft.postings[0].amount = 124400;
  EXPECT_EQ(books::stage(draft), books::DraftStage::unbalanced);
}

TEST(ParseTypedAmount, SignDecimalCommaOrPointAndSpaces) {
  EXPECT_EQ(parse_typed_amount("3,70"), Amount{370});
  EXPECT_EQ(parse_typed_amount("-3.70"), Amount{-370});
  EXPECT_EQ(parse_typed_amount("+1118"), Amount{111800});
  EXPECT_EQ(parse_typed_amount("-1 118,00"), Amount{-111800});
  EXPECT_EQ(parse_typed_amount("1\xC2\xA0" "118,5"), Amount{111850});
  EXPECT_FALSE(parse_typed_amount("tre"));
  EXPECT_FALSE(parse_typed_amount("1,234"));
  EXPECT_FALSE(parse_typed_amount(""));
}

TEST(ReplaceDraft, ChangesTheDraftSeenAndFollowsItsDate) {
  auto const folder = fs::temp_directory_path() / "cratchit_replace_draft_test";
  fs::remove_all(folder);
  fs::create_directories(folder);
  std::ofstream(folder / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20260101 20261231\r\n";
  auto plan = plan_project_from_sie(folder / "test.se");
  ASSERT_TRUE(plan) << plan.error();
  ASSERT_TRUE(create_project_from_sie(*plan, "2026-11-09"));
  Date const start{2026y, January, 1d};
  Date const today{2026y, November, 18d};
  ASSERT_TRUE(add_event_draft(folder, start, {"Telia", 111800, oct20}, today));
  ASSERT_TRUE(add_event_draft(folder, start, {"Loopia", 31125, Date{2026y, November, 1d}}, today));
  auto books = books::read_books(folder);
  ASSERT_TRUE(books);
  auto const telia = books->years.front().drafts[0];
  auto changed = telia;
  books::Posting posting;
  posting.account = "1930";
  posting.amount = -111800;
  changed.postings.push_back(posting);
  changed.event.date = Date{2026y, December, 1d}; // after Loopia now
  // Not the draft seen there: refused
  EXPECT_FALSE(replace_draft(folder, start, 1, telia, changed));
  // A date outside the year: refused
  auto outside = changed;
  outside.event.date = Date{2027y, January, 2d};
  auto const refused = replace_draft(folder, start, 0, telia, outside);
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "2027-01-02 ligger inte i räkenskapsåret 2026-01-01 – 2026-12-31.");
  // The one seen: changed and placed by its new date
  auto const at = replace_draft(folder, start, 0, telia, changed);
  ASSERT_TRUE(at) << at.error();
  EXPECT_EQ(*at, 1u);
  books = books::read_books(folder);
  ASSERT_TRUE(books);
  EXPECT_EQ(books->years.front().drafts[1], changed);
  fs::remove_all(folder);
}

TEST(BookDraft, AReadyDraftBecomesTheNextVoucherOfItsSeries) {
  auto const folder = fs::temp_directory_path() / "cratchit_book_draft_test";
  fs::remove_all(folder);
  fs::create_directories(folder);
  std::ofstream(folder / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20260101 20261231\r\n"
         "#KONTO 1930 \"Bank\"\r\n#KONTO 6570 \"Bankkostnader\"\r\n"
         "#VER \"A\" \"9\" 20260301 \"Avgift\"\r\n{\r\n#TRANS 6570 {} 1.00\r\n#TRANS 1930 {} -1.00\r\n}\r\n";
  auto plan = plan_project_from_sie(folder / "test.se");
  ASSERT_TRUE(plan) << plan.error();
  ASSERT_TRUE(create_project_from_sie(*plan, "2026-11-09"));
  Date const start{2026y, January, 1d};
  Date const today{2026y, November, 18d};
  ASSERT_TRUE(add_event_draft(folder, start, {"Kreditfaktura", -370, oct20}, today));
  auto books = books::read_books(folder);
  ASSERT_TRUE(books);
  EXPECT_EQ(next_number(books->years.front(), "A"), "10");
  EXPECT_EQ(next_number(books->years.front(), "D"), "1");
  auto const event = books->years.front().drafts[0];
  // Not ready: refused
  auto refused = book_draft(folder, start, 0, event, "A", today);
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "Utkastet är inte klart att bokföra.");
  auto ready = event;
  books::Posting bank;
  bank.account = "1930";
  bank.amount = 370;
  bank.text = "Återbetalning";
  bank.source = "bank.csv:3";
  books::Posting fee;
  fee.account = "6570";
  fee.amount = -370;
  ready.postings = {bank, fee};
  ASSERT_TRUE(replace_draft(folder, start, 0, event, ready));
  // Not a series of the books: refused
  refused = book_draft(folder, start, 0, ready, "Q", today);
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "Q är ingen serie i bokföringen.");
  // Booked: the next number, the amount without sign, registered today, the draft gone
  auto const voucher = book_draft(folder, start, 0, ready, "A", today);
  ASSERT_TRUE(voucher) << voucher.error();
  EXPECT_EQ(voucher->series, "A");
  EXPECT_EQ(voucher->number, "10");
  EXPECT_EQ(voucher->event, (books::Event{"Kreditfaktura", 370, oct20}));
  EXPECT_EQ(voucher->registered, today);
  ASSERT_EQ(voucher->postings.size(), 2u);
  EXPECT_EQ(voucher->postings[0].text, "Återbetalning");
  EXPECT_TRUE(voucher->postings[0].source.empty());
  books = books::read_books(folder);
  ASSERT_TRUE(books);
  EXPECT_TRUE(books->years.front().drafts.empty());
  ASSERT_EQ(books->years.front().vouchers.size(), 2u);
  EXPECT_EQ(books->years.front().vouchers.back(), *voucher);
  // Not the draft seen any more
  EXPECT_FALSE(book_draft(folder, start, 0, ready, "A", today));
  fs::remove_all(folder);
}

TEST(UnbookVoucher, TheLastOfASeriesBecomesADraftAndBooksAgainAsItWas) {
  auto const folder = fs::temp_directory_path() / "cratchit_unbook_voucher_test";
  fs::remove_all(folder);
  fs::create_directories(folder);
  std::ofstream(folder / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20260101 20261231\r\n"
         "#KONTO 1930 \"Bank\"\r\n#KONTO 6570 \"Bankkostnader\"\r\n"
         "#VER \"A\" \"1\" 20260301 \"Avgift mars\"\r\n{\r\n#TRANS 6570 {} 1.00\r\n#TRANS 1930 {} -1.00\r\n}\r\n"
         "#VER \"A\" \"2\" 20260401 \"Avgift april\"\r\n{\r\n#TRANS 6570 {} 2.00\r\n#TRANS 1930 {} -2.00 20260402 \"Nordea\"\r\n}\r\n";
  auto plan = plan_project_from_sie(folder / "test.se");
  ASSERT_TRUE(plan) << plan.error();
  ASSERT_TRUE(create_project_from_sie(*plan, "2026-11-09"));
  Date const start{2026y, January, 1d};
  Date const today{2026y, November, 18d};
  auto books = books::read_books(folder);
  ASSERT_TRUE(books);
  auto const& fiscal = books->years.front();
  auto const last_in_series = last_vouchers(fiscal);
  ASSERT_EQ(last_in_series.size(), 1u);
  EXPECT_EQ(last_in_series[0].number, "2");
  // Not the last of its series: refused
  auto const first = *std::ranges::find(fiscal.vouchers, std::string("1"), &books::Voucher::number);
  auto const refused = unbook_voucher(folder, start, first, today);
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "A1 är inte den sista verifikationen i serie A.");
  // The last: a ready draft, noted today, with the postings' dates and texts
  auto const index = unbook_voucher(folder, start, last_in_series[0], today);
  ASSERT_TRUE(index) << index.error();
  books = books::read_books(folder);
  ASSERT_TRUE(books);
  ASSERT_EQ(books->years.front().vouchers.size(), 1u);
  auto const draft = books->years.front().drafts[*index];
  EXPECT_EQ(draft.event, last_in_series[0].event);
  EXPECT_EQ(draft.noted, today);
  EXPECT_EQ(books::stage(draft), books::DraftStage::ready);
  ASSERT_EQ(draft.postings.size(), 2u);
  EXPECT_EQ(draft.postings[1].text, "Nordea");
  // Booked again in A: the same number
  auto const again = book_draft(folder, start, *index, draft, "A", today);
  ASSERT_TRUE(again) << again.error();
  EXPECT_EQ(again->number, "2");
  fs::remove_all(folder);
}

TEST(ReversalOf, TurnsEachPostingAndTheAmount) {
  books::Posting supplier;
  supplier.account = "2440";
  supplier.amount = -123375;
  supplier.text = "Telia";
  supplier.date = Date{2026y, October, 21d};
  books::Posting cost;
  cost.account = "6210";
  cost.amount = 98700;
  books::Posting vat;
  vat.account = "2641";
  vat.amount = 24675;
  books::Posting struck = cost;
  struck.kind = books::Posting::Kind::removed;
  books::Voucher const voucher{"D", "19", {"Telia faktura", 123375, oct20}, oct20, "KO", {supplier, cost, vat, struck}};
  auto const draft = reversal_of(voucher, Date{2026y, November, 18d});
  EXPECT_EQ(draft.event, (books::Event{"Ändring av D19", -123375, oct20}));
  EXPECT_EQ(draft.noted, (Date{2026y, November, 18d}));
  ASSERT_EQ(draft.postings.size(), 3u);
  EXPECT_EQ(draft.postings[0].account, "2440");
  EXPECT_EQ(draft.postings[0].amount, 123375);
  EXPECT_EQ(draft.postings[0].text, "Telia");
  EXPECT_FALSE(draft.postings[0].date);
  EXPECT_EQ(draft.postings[1].amount, -98700);
  EXPECT_EQ(draft.postings[2].amount, -24675);
  EXPECT_EQ(books::stage(draft), books::DraftStage::ready);
}

TEST(NewYear, TheBooksRunOnAndTheNewYearOpensWithTheBalances) {
  auto const folder = fs::temp_directory_path() / "cratchit_new_year_test";
  fs::remove_all(folder);
  fs::create_directories(folder);
  std::ofstream(folder / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20250101 20251231\r\n"
         "#KONTO 1930 \"Bank\"\r\n#KONTO 6570 \"Bankkostnader\"\r\n#IB 0 1930 1000.00\r\n"
         "#VER \"A\" \"1\" 20251201 \"Avgift\"\r\n{\r\n#TRANS 6570 {} 10.00\r\n#TRANS 1930 {} -10.00\r\n}\r\n";
  auto plan = plan_project_from_sie(folder / "test.se");
  ASSERT_TRUE(plan) << plan.error();
  ASSERT_TRUE(create_project_from_sie(*plan, "2026-11-09"));
  Date const start{2026y, January, 1d};
  Date const today{2026y, November, 18d};
  // A draft in the year after: the year is added
  auto const added = add_event_draft(folder, start, {"Avgift januari", 500, Date{2026y, January, 5d}}, today);
  ASSERT_TRUE(added) << added.error();
  auto books = books::read_books(folder);
  ASSERT_TRUE(books);
  ASSERT_EQ(books->years.size(), 2u);
  EXPECT_EQ(books->years[1].start, start);
  EXPECT_EQ(books->years[1].end, (Date{2026y, December, 31d}));
  ASSERT_EQ(books->years[1].drafts.size(), 1u);
  // It opens with the year before's closing balances: the real balance, 1000 - 10
  auto const balances = books::balances(*books);
  ASSERT_EQ(balances.size(), 2u);
  EXPECT_EQ(balances[1].opening.at("1930"), 99000);
  EXPECT_TRUE(balances[1].carried);
  EXPECT_FALSE(balances[1].opening.contains("6570")); // result accounts open at zero
  // Booked in the new year: A1 there, as numbering starts again
  auto draft = books->years[1].drafts[0];
  auto ready = draft;
  books::Posting bank;
  bank.account = "1930";
  bank.amount = -500;
  books::Posting fee;
  fee.account = "6570";
  fee.amount = 500;
  ready.postings = {bank, fee};
  ASSERT_TRUE(replace_draft(folder, start, 0, draft, ready));
  auto const voucher = book_draft(folder, start, 0, ready, "A", today);
  ASSERT_TRUE(voucher) << voucher.error();
  EXPECT_EQ(voucher->number, "1");
  books = books::read_books(folder);
  ASSERT_TRUE(books);
  EXPECT_EQ(books::balances(*books)[1].closing.at("1930"), 98500);
  // A closed year: not booked in, its vouchers not made drafts
  ASSERT_TRUE(add_event_draft(folder, start, {"Avgift februari", 500, Date{2026y, February, 5d}}, today));
  books = books::read_books(folder);
  ASSERT_TRUE(books);
  books->years[1].closed = true;
  ASSERT_TRUE(books::write_books(folder, *books));
  books = books::read_books(folder);
  ASSERT_TRUE(books);
  ASSERT_TRUE(books->years[1].closed);
  auto const february = books->years[1].drafts[0];
  auto february_ready = february;
  february_ready.postings = {bank, fee};
  ASSERT_TRUE(replace_draft(folder, start, 0, february, february_ready));
  auto const refused = book_draft(folder, start, 0, february_ready, "A", today);
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "Räkenskapsåret 2026-01-01 – 2026-12-31 är stängt: det går inte att bokföra i det.");
  books = books::read_books(folder);
  ASSERT_TRUE(books);
  auto const unbooked = unbook_voucher(folder, start, books->years[1].vouchers[0], today);
  ASSERT_FALSE(unbooked);
  EXPECT_EQ(unbooked.error(),
            "Räkenskapsåret 2026-01-01 – 2026-12-31 är stängt: dess verifikationer går inte att göra till utkast.");
  fs::remove_all(folder);
}
