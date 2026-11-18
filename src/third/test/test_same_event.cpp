#include "core/books/books.hpp"
#include "core/event_draft.hpp"
#include "core/same_event.hpp"
#include "core/sie_project.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace cratchit;
using namespace std::chrono;

namespace {

Date on(int d, unsigned m = 9) { return Date{2026y, month{m}, std::chrono::day{static_cast<unsigned>(d)}}; }

books::Posting posting(std::string account, Amount amount, std::string text = {}, std::string source = {}) {
  books::Posting p;
  p.account = std::move(account);
  p.amount = amount;
  p.text = std::move(text);
  p.source = std::move(source);
  return p;
}

books::Draft event_draft(std::string heading, Amount amount, Date date) {
  return {{std::move(heading), amount, date}, on(17, 11), {}};
}

books::Draft statement_draft(std::string account, Amount amount, Date date, std::string text, std::string source) {
  return {{"", 0, date}, on(17, 11), {posting(std::move(account), amount, std::move(text), std::move(source))}};
}

books::FiscalYear year_of(std::vector<books::Draft> drafts, std::vector<books::Voucher> vouchers = {}) {
  books::FiscalYear year{};
  year.start = Date{2026y, May, 1d};
  year.end = Date{2027y, April, 30d};
  year.vouchers = std::move(vouchers);
  for (auto& draft : drafts) year.place_draft(std::move(draft));
  return year;
}

} // namespace

TEST(SameEvent, AmountWithoutSignAndTextToCompare) {
  EXPECT_EQ(amount_of(event_draft("Kreditfaktura", -124500, on(1))), 124500);
  auto const statement = statement_draft("1920", -130875, on(28), "LOOPIA AB (WEBSUPPORT) | Webhotell", "a.csv:3");
  EXPECT_EQ(amount_of(statement), 130875);
  EXPECT_EQ(compared_text(statement), "LOOPIA AB (WEBSUPPORT) | Webhotell");
  EXPECT_EQ(compared_text(event_draft("Loopia webhotell", 130875, on(28))), "Loopia webhotell");
  // Case folded, no numbers, no one-letter words
  EXPECT_EQ(shared_words("Loopia webhotell 2026 a", "5343-2795 | LOOPIA AB | Webhotell 12/10 a"),
            (std::vector<std::string>{"loopia", "webhotell"}));
  EXPECT_EQ(shared_words("Årsavgift", "ÅRSAVGIFT"), (std::vector<std::string>{"årsavgift"}));
}

TEST(SameEvent, TheSameAmountWithinFourDaysTheFittingFirst) {
  auto const year = year_of({
      event_draft("Loopia webhotell", 130875, on(26)),                                        // #1
      statement_draft("1920", -130875, on(28), "LOOPIA AB | Webhotell", "nordea.csv:7"),      // #2
      event_draft("Telia", 130875, on(30)),                                                    // #4: 2 days from #2
      event_draft("Clas Ohlson", 24900, on(28)),                                              // #3: another amount, the same day as #2
      event_draft("För långt bort", 130875, on(5, 10)),                                       // #5: too far
  });
  auto const pairs = find_same_events(year);
  ASSERT_EQ(pairs.size(), 2u);
  // An event and a statement entry with shared words first
  EXPECT_EQ(pairs[0], (SameEvent{SameEvent::Kind::drafts, 0, 1, 2, {"loopia", "webhotell"}, true}));
  // Telia and the entry (event and postings), no shared words
  EXPECT_EQ(pairs[1], (SameEvent{SameEvent::Kind::drafts, 1, 3, 2, {}, true}));
  // Two events (Loopia and Telia) are never a pair: one of them would be lost
}

TEST(SameEvent, PostingsThatBalanceTogetherFitButNotOnTheSameAccount) {
  // A transfer: one entry on each statement
  auto year = year_of({statement_draft("1920", -500000, on(3), "Till sparkonto", "pg.csv:2"),
                       statement_draft("1940", 500000, on(4), "Från PlusGiro", "spar.csv:2")});
  auto pairs = find_same_events(year);
  ASSERT_EQ(pairs.size(), 1u);
  EXPECT_TRUE(pairs[0].fits);
  // Both on the tax account: they cancel out, two events in a row; not a pair
  year = year_of({statement_draft("1630", 229200, on(18, 7), "Moms", "skv.csv:2"),
                  statement_draft("1630", -229200, on(21, 7), "Utbetalning", "skv.csv:3")});
  EXPECT_TRUE(find_same_events(year).empty());
  // Both out of an account: two events, not a pair
  year = year_of({statement_draft("1920", -500000, on(3), "Till sparkonto", "pg.csv:2"),
                  statement_draft("1940", -500000, on(4), "Uttag", "spar.csv:2")});
  EXPECT_TRUE(find_same_events(year).empty());
  // An event merged with its payment, and another event of the same amount: not a pair
  auto paid = event_draft("Loopia webhotell", 130875, on(28));
  paid.postings = {posting("1920", -130875, "LOOPIA", "nordea.csv:7")};
  year = year_of({paid, event_draft("Loopia webhotell", 130875, on(28))});
  EXPECT_TRUE(find_same_events(year).empty());
  // ... but it pairs with an entry that balances its posting (signs of an event are not compared)
  year = year_of({paid, statement_draft("1940", 130875, on(29), "Från PlusGiro", "spar.csv:3")});
  ASSERT_EQ(find_same_events(year).size(), 1u);
  year = year_of({event_draft("Kundbetalning", 50000, on(3)), statement_draft("1920", 50000, on(3), "Kund", "pg.csv:4")});
  EXPECT_EQ(find_same_events(year).size(), 1u);
}

TEST(SameEvent, ADraftAndAVoucherInTheBooks) {
  books::Voucher voucher{"D", "6", {"Loopia Faktura", 31125, on(31, 8)}, std::nullopt, "",
                         {posting("2440", -31125), posting("6540", 24900), posting("2641", 6225)}};
  books::Voucher paid{"D", "7", {"Loopia betalning", 31125, on(1)}, std::nullopt, "",
                      {posting("1920", -31125), posting("2440", 31125)}};
  auto const year = year_of({statement_draft("1920", -31125, on(1), "LOOPIA AB", "nordea.csv:4")}, {voucher, paid});
  auto const pairs = find_same_events(year);
  ASSERT_EQ(pairs.size(), 2u);
  // The payment has the entry's posting: it fits and comes first
  EXPECT_EQ(pairs[0], (SameEvent{SameEvent::Kind::booked, 0, 1, 0, {"loopia"}, true}));
  EXPECT_EQ(pairs[1], (SameEvent{SameEvent::Kind::booked, 0, 0, 1, {"loopia"}, false}));
  EXPECT_TRUE(unbooked(year.drafts[0], paid).empty());
  EXPECT_EQ(unbooked(year.drafts[0], voucher).size(), 1u);
}

TEST(SameEvent, MergedHasTheEventTheEarlierNotedDayAndEachEntryOnce) {
  auto a = statement_draft("1920", -130875, on(28), "LOOPIA", "nordea.csv:7");
  a.noted = on(10, 11);
  auto b = event_draft("Loopia webhotell", 130875, on(26));
  b.postings = {posting("1920", -130875, "LOOPIA", "nordea.csv:7"), posting("6540", 130875)};
  auto const one = merged(a, b, b.event);
  EXPECT_EQ(one.event, b.event);
  EXPECT_EQ(one.noted, on(10, 11));
  ASSERT_EQ(one.postings.size(), 2u);
  EXPECT_EQ(one.postings[0].account, "1920");
  EXPECT_EQ(one.postings[1].account, "6540");
}

TEST(SameEvent, MergeDraftsWritesTheOneDraftOnlyForTheDraftsSeen) {
  auto const folder = fs::temp_directory_path() / "cratchit_merge_drafts_test";
  fs::remove_all(folder);
  fs::create_directories(folder);
  std::ofstream(folder / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20260101 20261231\r\n";
  auto plan = plan_project_from_sie(folder / "test.se");
  ASSERT_TRUE(plan) << plan.error();
  ASSERT_TRUE(create_project_from_sie(*plan, "2026-11-09"));
  Date const start{2026y, January, 1d};
  ASSERT_TRUE(add_event_draft(folder, start, {"Telia", 111800, on(20, 10)}, on(17, 11)));
  ASSERT_TRUE(add_event_draft(folder, start, {"Telia betald", 111800, on(22, 10)}, on(16, 11)));
  ASSERT_TRUE(add_event_draft(folder, start, {"Loopia", 31125, on(1, 11)}, on(17, 11)));
  auto const books = books::read_books(folder);
  ASSERT_TRUE(books);
  auto const drafts = books->years.front().drafts;
  // Not the drafts seen at those places: refused, nothing changes
  auto const refused = merge_drafts(folder, start, 0, drafts[2], 1, drafts[1], drafts[0].event);
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "Utkasten finns inte längre på sina platser i bokföringen. Välj paret igen från Samma?.");
  EXPECT_FALSE(merge_drafts(folder, start, 1, drafts[1], 1, drafts[1], drafts[1].event));
  // The ones seen
  auto const index = merge_drafts(folder, start, 0, drafts[0], 1, drafts[1], drafts[1].event);
  ASSERT_TRUE(index) << index.error();
  EXPECT_EQ(*index, 0u);
  auto const after = books::read_books(folder);
  ASSERT_TRUE(after);
  ASSERT_EQ(after->years.front().drafts.size(), 2u);
  EXPECT_EQ(after->years.front().drafts[0].event.heading, "Telia betald");
  EXPECT_EQ(after->years.front().drafts[0].noted, on(16, 11));
  EXPECT_EQ(after->years.front().drafts[1].event.heading, "Loopia");
  fs::remove_all(folder);
}

TEST(DraftParts, LoneSameOrMore) {
  EXPECT_EQ(parts_of(event_draft("Telia", 111800, on(20, 10))), DraftParts::lone);
  EXPECT_EQ(parts_of(statement_draft("1920", -111800, on(22, 10), "TELIA", "a.csv:2")), DraftParts::lone);
  // An event and its payment, a transfer: the same amount without sign
  auto paid = event_draft("Telia", 111800, on(20, 10));
  paid.postings = {posting("1920", -111800, "TELIA", "a.csv:2")};
  EXPECT_EQ(parts_of(paid), DraftParts::same);
  books::Draft transfer{{"", 0, on(3)}, on(17, 11), {posting("1920", -500000), posting("1940", 500000)}};
  EXPECT_EQ(parts_of(transfer), DraftParts::same);
  // A credit note keeps its sign, still the same
  auto credit = event_draft("Kreditfaktura", -124500, on(20, 10));
  credit.postings = {posting("1920", 124500)};
  EXPECT_EQ(parts_of(credit), DraftParts::same);
  // Postings of different amounts: more than the same
  paid.postings = {posting("2440", 111800), posting("6212", -89440), posting("2641", -22360)};
  EXPECT_EQ(parts_of(paid), DraftParts::more);
}

TEST(DraftParts, SplitGivesTheEventAndEachAccountEventNotedAsTheDraft) {
  auto draft = event_draft("Loopia webhotell", 130875, on(26));
  draft.noted = on(10, 11);
  draft.postings = {posting("1920", -130875, "LOOPIA", "nordea.csv:7")};
  draft.postings[0].date = on(28);
  auto const parts = split(draft);
  ASSERT_EQ(parts.size(), 2u);
  EXPECT_EQ(parts[0], (books::Draft{draft.event, on(10, 11), {}}));
  EXPECT_EQ(parts[1], (books::Draft{{"", 0, on(28)}, on(10, 11), draft.postings}));
  EXPECT_EQ(books::stage(parts[0]), books::DraftStage::event);
  EXPECT_EQ(books::stage(parts[1]), books::DraftStage::account_event);
  // A merged draft split gives the drafts it was made of (the event chosen, the entries)
  auto const event = event_draft("Loopia webhotell", 130875, on(26));
  auto entry = statement_draft("1920", -130875, on(28), "LOOPIA", "nordea.csv:7");
  entry.postings[0].date = on(28); // as a statement entry's
  EXPECT_EQ(split(merged(event, entry, event.event)), (std::vector<books::Draft>{event, entry}));
}

TEST(DraftParts, SplitDraftWritesTheLoneDraftsOnlyForTheSame) {
  auto const folder = fs::temp_directory_path() / "cratchit_split_draft_test";
  fs::remove_all(folder);
  fs::create_directories(folder);
  std::ofstream(folder / "test.se", std::ios::binary)
      << "#FLAGGA 0\r\n#SIETYP 4\r\n#FNAMN \"Test AB\"\r\n#ORGNR 556000-0001\r\n#RAR 0 20260101 20261231\r\n";
  auto plan = plan_project_from_sie(folder / "test.se");
  ASSERT_TRUE(plan) << plan.error();
  ASSERT_TRUE(create_project_from_sie(*plan, "2026-11-09"));
  Date const start{2026y, January, 1d};
  ASSERT_TRUE(add_event_draft(folder, start, {"Telia", 111800, on(20, 10)}, on(17, 11)));
  ASSERT_TRUE(add_event_draft(folder, start, {"Telia betald", 111800, on(22, 10)}, on(17, 11)));
  auto books = books::read_books(folder);
  ASSERT_TRUE(books);
  auto const drafts = books->years.front().drafts;
  // A lone draft is not split
  auto refused = split_draft(folder, start, 0, drafts[0]);
  ASSERT_FALSE(refused);
  EXPECT_EQ(refused.error(), "Utkastet är inte gjort av delar med samma belopp och kan inte delas.");
  ASSERT_TRUE(merge_drafts(folder, start, 0, drafts[0], 1, drafts[1], drafts[0].event));
  books = books::read_books(folder);
  ASSERT_TRUE(books);
  auto const one = books->years.front().drafts[0];
  // Merging two events keeps one event: split gives that one alone
  EXPECT_EQ(parts_of(one), DraftParts::lone);
  // Not the draft seen: refused
  EXPECT_FALSE(split_draft(folder, start, 0, drafts[1]));
  fs::remove_all(folder);
}

TEST(DraftRemoval, ForgetWithoutStatementEntriesElseSplitOrNothing) {
  // No statement entries: always forget, even more than the same
  auto typed = event_draft("Telia", 111800, on(20, 10));
  typed.postings = {posting("2440", -111800), posting("6212", 89440), posting("2641", 22360)};
  EXPECT_EQ(parts_of(typed), DraftParts::more);
  EXPECT_EQ(removal_of(typed), DraftRemoval::forget);
  // A lone statement entry: forget
  auto const entry = statement_draft("1920", -111800, on(22, 10), "TELIA", "a.csv:2");
  EXPECT_EQ(removal_of(entry), DraftRemoval::forget);
  // With a statement entry, the same: split; more than the same: nothing
  auto paid = event_draft("Telia", 111800, on(20, 10));
  paid.postings = entry.postings;
  EXPECT_EQ(removal_of(paid), DraftRemoval::split);
  paid.postings.push_back(posting("6212", 89440));
  EXPECT_EQ(removal_of(paid), DraftRemoval::none);
}
