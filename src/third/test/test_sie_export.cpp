#include "core/books/export.hpp"
#include "core/books/import.hpp"
#include "core/sie/check.hpp"
#include "core/sie/file.hpp"
#include "core/sie/writer.hpp"
#include "core/version.hpp"
#include "sie_test_file.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <format>
#include <string>
#include <vector>

using namespace cratchit;
using namespace std::chrono;
using sie_test::balanced_balances;
using sie_test::parse_sie;
using sie_test::sale_and_cost;
using sie_test::sie_file;

namespace {

books::ImportPlan imported(books::Books const& books, books::Identity const& identity, std::string const& text) {
  auto plan = books::plan_import(books, identity, parse_sie(text));
  for (auto const& error : plan.errors) {
    ADD_FAILURE() << error;
  }
  return plan;
}

// 2024: 1000 into the bank (A 1); 2025: a sale of 500 and bank costs of 10 (A 1, A 2), as imported
books::ImportPlan two_years() {
  auto const first = imported({}, {"Test", ""},
                              sie_file(2024,
                                       "#IB 0 1930 0\r\n#UB 0 1930 1000\r\n#UB 0 2099 -1000\r\n",
                                       "#VER A 1 20240601 \"Capital\" 20240602\r\n{\r\n"
                                       "#TRANS 1930 {} 1000\r\n#TRANS 2099 {} -1000\r\n}\r\n"));
  return imported(first.books, first.identity, sie_file(2025, balanced_balances(), sale_and_cost(2025)));
}

// The document an exported file reads back as, through the import's own path
sie::Document read_back(sie::Document const& document) {
  auto const encoded = sie::cp437_of(sie::text_of(document));
  EXPECT_EQ(encoded.unmapped, 0u);
  auto const lexed = sie::lex_bytes(encoded.bytes);
  EXPECT_TRUE(lexed) << lexed.error();
  auto parsed = sie::parse_document(lexed->lines);
  for (auto const& error : parsed.errors) {
    ADD_FAILURE() << error;
  }
  return std::move(parsed.document);
}

} // namespace

TEST(SieWriter, RecordsAsSieWritesThem) {
  sie::Document document;
  document.flag = "0";
  document.format = "PC8";
  document.sie_type = "4";
  document.program = "cratchit";
  document.program_version = "0.1.0";
  document.generated = 2026y / October / 28;
  document.company_id = "503072";
  document.company_name = "Test AB";
  document.orgnr = "556000-0001";
  document.address = {"Kjell", "Gatan 1", "17141 Solna", ""};
  document.years = {{0, 2026y / May / 1, 2027y / April / 30}};
  document.covers_until = 2027y / April / 30;
  document.chart_type = "EUBAS97";
  document.accounts = {{"1930", "Bank", "", "7281", ""}, {"3001", "Sales", "I", "", "st"}};
  document.dimensions = {{"1", "Kostnadsställe", ""}, {"7", "Sub", "1"}};
  document.objects = {{"1", "100", "Lager"}};
  document.opening = {{0, "1930", 100000, ""}};
  document.closing = {{0, "1930", 129970, ""}};
  document.results = {{0, "3001", -29970, ""}};
  sie::Voucher voucher{"A", "1", 2026y / May / 5, "Sale \"big\"", 2026y / May / 6, "", {}, 0};
  voucher.transactions = {
      {sie::Transaction::Kind::normal, "1930", {}, 129970, {}, "", "0", ""},
      {sie::Transaction::Kind::normal, "3001", {{"1", "100"}}, -130000, 2026y / May / 4, "Row", "", ""},
      {sie::Transaction::Kind::added, "3001", {}, 30, {}, "Fix", "", "Kjell"},
      {sie::Transaction::Kind::removed, "1930", {}, 5, {}, "", "", "Kjell"},
  };
  document.vouchers = {voucher};
  EXPECT_EQ(sie::text_of(document), "#FLAGGA 0\r\n"
                                    "#FORMAT PC8\r\n"
                                    "#SIETYP 4\r\n"
                                    "#PROGRAM \"cratchit\" 0.1.0\r\n"
                                    "#GEN 20261028\r\n"
                                    "#FNR 503072\r\n"
                                    "#FNAMN \"Test AB\"\r\n"
                                    "#ADRESS \"Kjell\" \"Gatan 1\" \"17141 Solna\" \"\"\r\n"
                                    "#RAR 0 20260501 20270430\r\n"
                                    "#ORGNR 556000-0001\r\n"
                                    "#OMFATTN 20270430\r\n"
                                    "#KPTYP EUBAS97\r\n"
                                    "\r\n"
                                    "#KONTO 1930 \"Bank\"\r\n"
                                    "#SRU 1930 7281\r\n"
                                    "#KONTO 3001 \"Sales\"\r\n"
                                    "#KTYP 3001 I\r\n"
                                    "#ENHET 3001 \"st\"\r\n"
                                    "\r\n"
                                    "#DIM 1 \"Kostnadsställe\"\r\n"
                                    "#UNDERDIM 7 \"Sub\" 1\r\n"
                                    "#OBJEKT 1 \"100\" \"Lager\"\r\n"
                                    "\r\n"
                                    "#IB 0 1930 1000\r\n"
                                    "#UB 0 1930 1299.7\r\n"
                                    "\r\n"
                                    "#RES 0 3001 -299.7\r\n"
                                    "\r\n"
                                    "#VER A 1 20260505 \"Sale \\\"big\\\"\" 20260506\r\n"
                                    "{\r\n"
                                    "#TRANS 1930 {} 1299.7 \"\" \"\" 0\r\n"
                                    "#TRANS 3001 {1 \"100\"} -1300 20260504 \"Row\"\r\n"
                                    "#RTRANS 3001 {} 0.3 \"\" \"Fix\" \"\" \"Kjell\"\r\n"
                                    "#TRANS 3001 {} 0.3 \"\" \"Fix\"\r\n"
                                    "#BTRANS 1930 {} 0.05 \"\" \"\" \"\" \"Kjell\"\r\n"
                                    "}\r\n");
}

TEST(SieWriter, TextReadsBackAsTheSameDocument) {
  auto const document = books::export_document(two_years().books, {"Test AB", "556000-0001"}, 1, 2026y / October / 28);
  auto const back = read_back(document);
  EXPECT_EQ(back.company_name, "Test AB");
  EXPECT_EQ(back.orgnr, "556000-0001");
  EXPECT_EQ(back.program, "cratchit");
  EXPECT_EQ(back.vouchers, document.vouchers);
  EXPECT_EQ(back.accounts.size(), document.accounts.size());
}

TEST(SieWriter, Cp437) {
  auto const encoded = sie::cp437_of("Högdal Ä");
  EXPECT_EQ(encoded.bytes, (std::vector<std::uint8_t>{'H', 0x94, 'g', 'd', 'a', 'l', ' ', 0x8E}));
  EXPECT_EQ(encoded.unmapped, 0u);
}

TEST(SieWriter, Cp437WritesTheCodePointOfACharacterItLacks) {
  // The euro sign, a Polish ł and an emoji beyond U+FFFF
  auto const encoded = sie::cp437_of("100 € Łódź 😀");
  std::string const text(encoded.bytes.begin(), encoded.bytes.end());
  EXPECT_EQ(text, "100 <U+20AC> <U+0141>\xA2" "d<U+017A> <U+1F600>");
  EXPECT_EQ(encoded.unmapped, 4u);
}

TEST(SieWriter, Cp437MarksBytesThatAreNotUtf8) {
  auto const encoded = sie::cp437_of("a\xFF" "b");
  EXPECT_EQ(std::string(encoded.bytes.begin(), encoded.bytes.end()), "a<U+FFFD>b");
  EXPECT_EQ(encoded.unmapped, 1u);
}

TEST(SieExport, BalancesAsCratchitCalculatesThem) {
  auto const plan = two_years();
  auto const document = books::export_document(plan.books, plan.identity, 1, 2026y / October / 28);
  ASSERT_EQ(document.years.size(), 2u);
  EXPECT_EQ(document.years[0].start, 2025y / January / 1);
  EXPECT_EQ(document.years[1].start, 2024y / January / 1);
  EXPECT_EQ(document.program_version, std::string(version()));
  EXPECT_EQ(document.covers_until, 2025y / December / 31);
  auto const text = sie::text_of(document);
  EXPECT_NE(text.find("#IB 0 1930 1000\r\n#UB 0 1930 1490\r\n#IB 0 2099 -1000\r\n#UB 0 2099 -1000\r\n\r\n"
                      "#IB -1 1930 0\r\n#UB -1 1930 1000\r\n#IB -1 2099 0\r\n#UB -1 2099 -1000\r\n\r\n"
                      "#RES 0 3010 -500\r\n#RES 0 6570 10\r\n\r\n"
                      "#VER A 1 20250201 \"Sale\" 20250205\r\n"),
            std::string::npos)
      << text;
  // Only year 0's vouchers
  EXPECT_EQ(document.vouchers.size(), 2u);
}

TEST(SieExport, TheFirstYearHasNoYearBefore) {
  auto const plan = two_years();
  auto const document = books::export_document(plan.books, plan.identity, 0, 2026y / October / 28);
  ASSERT_EQ(document.years.size(), 1u);
  EXPECT_EQ(document.vouchers.size(), 1u);
}

TEST(SieExport, ImportsBackAsTheSameBooks) {
  auto plan = two_years();
  auto books = plan.books;
  // An entered voucher with a non-ASCII heading, objects, and a row added and one removed after registration
  books.years[1].vouchers.push_back(
      {"D", "1", {"Räkning från \"Åhléns\"", 12600, 2025y / June / 1}, 2025y / June / 2, "", {
          {books::Posting::Kind::normal, "6570", {{"1", "100"}}, 12500, {}, "Kostnad", "", ""},
          {books::Posting::Kind::normal, "1930", {}, -12500, {}, "", "", ""},
          {books::Posting::Kind::added, "6570", {}, 100, 2025y / June / 3, "Rättelse", "", "KO"},
          {books::Posting::Kind::added, "1930", {}, -100, {}, "", "", "KO"},
          {books::Posting::Kind::removed, "6570", {}, 7, {}, "", "", "KO"},
      }});
  for (std::size_t index = 0; index < books.years.size(); ++index) {
    auto const document = books::export_document(books, plan.identity, index, 2026y / October / 28);
    auto const back = read_back(document);
    auto const findings = sie::check_document(back);
    EXPECT_TRUE(findings.errors.empty()) << findings.errors.front();
    // Into the books it came from: nothing new, nothing differing
    auto const again = books::plan_import(books, plan.identity, back);
    EXPECT_TRUE(again.errors.empty()) << again.errors.front();
    EXPECT_EQ(again.vouchers_added, 0u);
    ASSERT_EQ(again.books.years.size(), books.years.size());
    for (std::size_t y = 0; y < books.years.size(); ++y) {
      EXPECT_EQ(again.books.years[y].vouchers, books.years[y].vouchers);
    }
  }
  // Both years into empty books give the same vouchers and accounts
  auto const first = books::plan_import({}, {"Test", ""}, read_back(books::export_document(books, plan.identity, 0, 2026y / October / 28)));
  auto const second = books::plan_import(first.books, first.identity,
                                         read_back(books::export_document(books, plan.identity, 1, 2026y / October / 28)));
  ASSERT_TRUE(second.errors.empty()) << second.errors.front();
  ASSERT_EQ(second.books.years.size(), 2u);
  EXPECT_EQ(second.books.years[0].vouchers, books.years[0].vouchers);
  EXPECT_EQ(second.books.years[1].vouchers, books.years[1].vouchers);
  EXPECT_EQ(second.books.accounts, books.accounts);
  EXPECT_EQ(second.identity.orgnr, "556000-0001");
}

TEST(SieExport, FileNamedAsTheYearsPrompt) {
  books::FiscalYear broken{.start = 2026y / May / 1, .end = 2027y / April / 30};
  books::FiscalYear calendar{.start = 2025y / January / 1, .end = 2025y / December / 31};
  EXPECT_EQ(books::export_file_name(broken, books::ExportKind::all), "sie_2026-27_allt.se");
  EXPECT_EQ(books::export_file_name(calendar, books::ExportKind::all), "sie_2025_allt.se");
  EXPECT_EQ(books::export_file_name(broken, books::ExportKind::all, 2), "sie_2026-27_allt_2.se");
  EXPECT_EQ(books::export_file_name(calendar, books::ExportKind::diff), "sie_2025_skillnad.se");
  EXPECT_EQ(books::export_file_name(calendar, books::ExportKind::diff, 3), "sie_2025_skillnad_3.se");
}

TEST(SieExport, APartOfTheYearHasNoBalances) {
  auto const plan = two_years();
  auto const document = books::export_document(plan.books, plan.identity, 1, 2026y / October / 28,
                                               std::vector<std::size_t>{1});
  ASSERT_EQ(document.vouchers.size(), 1u);
  EXPECT_EQ(document.vouchers[0].number, "2");
  EXPECT_TRUE(document.opening.empty());
  EXPECT_TRUE(document.closing.empty());
  EXPECT_TRUE(document.results.empty());
  EXPECT_EQ(document.years.size(), 2u);
  EXPECT_FALSE(document.accounts.empty());
}

TEST(SieExport, DiffWithAFileByDateAndRows) {
  auto plan = two_years();
  auto& books = plan.books;
  books.years[1].vouchers.push_back({"D", "1", {"Lunch", 2000, 2025y / June / 1}, 2025y / June / 1, "", {
      {books::Posting::Kind::normal, "6570", {}, 2000, {}, "", "", ""},
      {books::Posting::Kind::normal, "1930", {}, -2000, {}, "", "", ""}}});
  // The file numbered A 1 as A 7: still the same voucher
  auto renumbered = sale_and_cost(2025);
  renumbered.replace(renumbered.find("#VER A 1 "), 9, "#VER A 7 ");
  auto const diff = books::diff_with(books, 1, plan.identity, parse_sie(sie_file(2025, balanced_balances(), renumbered)));
  ASSERT_TRUE(diff.has_value()) << diff.error();
  EXPECT_EQ(diff->books_only, std::vector<std::size_t>{2});
  EXPECT_TRUE(diff->file_only.empty());
  // A voucher the books lack
  auto const ahead = books::diff_with(
      books, 1, plan.identity,
      parse_sie(sie_file(2025, balanced_balances(),
                         sale_and_cost(2025) + "#VER A 3 20250701 \"Fee\"\r\n{\r\n#TRANS 6570 {} 5\r\n"
                                               "#TRANS 1930 {} -5\r\n}\r\n")));
  ASSERT_TRUE(ahead.has_value()) << ahead.error();
  ASSERT_EQ(ahead->file_only.size(), 1u);
  EXPECT_EQ(ahead->file_only[0].number, "3");
  // Another year or company
  auto const year = books::diff_with(books, 0, plan.identity, parse_sie(sie_file(2025, "", "")));
  EXPECT_EQ(year.error(), "The file's fiscal year 0 is 2025-01-01 - 2025-12-31, not 2024-01-01 - 2024-12-31");
  auto const other = books::diff_with(books, 1, plan.identity,
                                      parse_sie(sie_file(2025, "", "", "\"Other AB\"", "556111-2222")));
  EXPECT_EQ(other.error(), "The file is for organisation number 556111-2222, but the project Test AB has 556000-0001");
}
