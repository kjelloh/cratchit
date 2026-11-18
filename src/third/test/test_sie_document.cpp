#include "core/sie/document.hpp"

#include "sie_test_file.hpp"

#include <gtest/gtest.h>

#include <format>
#include <string>
#include <string_view>

using namespace std::chrono;
using cratchit::sie::Crc32;
using cratchit::sie::Transaction;
using sie_test::parse_errors;
using sie_test::parse_sie;

namespace {

std::uint32_t crc_of(std::string_view text) {
  Crc32 crc;
  for (char const c : text) {
    crc.add(static_cast<std::uint8_t>(c));
  }
  return crc.value();
}

} // namespace

TEST(SieDocument, IdentificationRecords) {
  auto const document = parse_sie("#FLAGGA 0\n#FORMAT PC8\n#SIETYP 4\n#PROGRAM \"Fortnox\" 3.62.17\n#GEN 20261019 KO\n"
                                  "#FNR 503072\n#FNAMN \"The ITfied AB\"\n#ORGNR 556782-8172\n"
                                  "#ADRESS \"Kim\" \"Street 1\" \"17141 Solna\" \"070\"\n"
                                  "#RAR 0 20260501 20270430\n#RAR -1 20250501 20260430\n#OMFATTN 20270430\n"
                                  "#KPTYP EUBAS97\n#VALUTA SEK\n#TAXAR 2026\n#FTYP AB\n#BKOD 62010\n#PROSA \"Hello\"\n");
  EXPECT_EQ(document.flag, "0");
  EXPECT_EQ(document.format, "PC8");
  EXPECT_EQ(document.sie_type, "4");
  EXPECT_EQ(document.program, "Fortnox");
  EXPECT_EQ(document.program_version, "3.62.17");
  EXPECT_EQ(document.generated, year_month_day(year{2026}, October, day{19}));
  EXPECT_EQ(document.generated_by, "KO");
  EXPECT_EQ(document.company_id, "503072");
  EXPECT_EQ(document.company_name, "The ITfied AB");
  EXPECT_EQ(document.orgnr, "556782-8172");
  EXPECT_EQ(document.address.street, "Street 1");
  EXPECT_EQ(document.address.phone, "070");
  ASSERT_EQ(document.years.size(), 2u);
  EXPECT_EQ(document.year(-1)->start, year_month_day(year{2025}, May, day{1}));
  EXPECT_EQ(document.year(0)->end, year_month_day(year{2027}, April, day{30}));
  EXPECT_FALSE(document.year(-2).has_value());
  EXPECT_EQ(document.covers_until, year_month_day(year{2027}, April, day{30}));
  EXPECT_EQ(document.chart_type, "EUBAS97");
  EXPECT_EQ(document.currency, "SEK");
  EXPECT_EQ(document.tax_year, "2026");
  EXPECT_EQ(document.company_type, "AB");
  EXPECT_EQ(document.sni_code, "62010");
  EXPECT_EQ(document.notes, std::vector<std::string>{"Hello"});
}

TEST(SieDocument, ChartOfAccounts) {
  auto const document = parse_sie("#KONTO 1930 \"Bank\"\n#KTYP 1930 T\n#SRU 1930 7281\n#ENHET 1930 st\n"
                                  "#KONTO 3010 \"Sales\"\n#KONTO 3010 \"Sales Sweden\"\n#SRU 4000 1\n");
  ASSERT_EQ(document.accounts.size(), 3u);
  EXPECT_EQ(document.accounts[0].number, "1930");
  EXPECT_EQ(document.accounts[0].name, "Bank");
  EXPECT_EQ(document.accounts[0].type, "T");
  EXPECT_EQ(document.accounts[0].sru, "7281");
  EXPECT_EQ(document.accounts[0].unit, "st");
  EXPECT_EQ(document.accounts[1].name, "Sales Sweden"); // given again, the last wins
  EXPECT_EQ(document.accounts[2].number, "4000");        // #SRU before #KONTO still counts
  EXPECT_EQ(document.accounts[2].name, "");
}

TEST(SieDocument, DimensionsAndObjects) {
  auto const document = parse_sie("#DIM 1 \"Kostnadsställe\"\n#UNDERDIM 21 \"Avdelning\" 1\n#OBJEKT 1 \"100\" \"Sälj\"\n");
  ASSERT_EQ(document.dimensions.size(), 2u);
  EXPECT_EQ(document.dimensions[1].super, "1");
  ASSERT_EQ(document.objects.size(), 1u);
  EXPECT_EQ(document.objects[0].number, "100");
}

TEST(SieDocument, Balances) {
  auto const document = parse_sie("#IB 0 1930 23780.78\n#UB -1 1930 23780.78 5\n#RES 0 3010 -500\n"
                                  "#OIB 0 1930 {1 \"100\"} 10\n#OUB 0 1930 {} 20\n"
                                  "#PSALDO 0 202605 1226 {} 5199.07 0\n#PBUDGET 0 202605 3010 {1 2} -100\n");
  ASSERT_EQ(document.opening.size(), 1u);
  EXPECT_EQ(document.opening[0].amount, 2378078);
  EXPECT_EQ(document.closing[0].year, -1);
  EXPECT_EQ(document.closing[0].quantity, "5");
  EXPECT_EQ(document.results[0].amount, -50000);
  ASSERT_EQ(document.object_opening.size(), 1u);
  EXPECT_EQ(document.object_opening[0].objects, (cratchit::sie::Objects{{"1", "100"}}));
  EXPECT_EQ(document.object_closing[0].amount, 2000);
  ASSERT_EQ(document.period_balances.size(), 1u);
  EXPECT_EQ(document.period_balances[0].period, "202605");
  EXPECT_EQ(document.period_budgets[0].objects, (cratchit::sie::Objects{{"1", "2"}}));
}

TEST(SieDocument, VoucherWithTransactions) {
  auto const document = parse_sie("#VER A 1 20260505 \"Kassa - PG\" 20260715 KO\n{\n"
                                  "#TRANS 1930 {} -13000 \"\" \"\" 0\n"
                                  "#TRANS 1920 {1 \"100\"} 13000 20260506 \"In\" 2 \"Kim\"\n}\n");
  ASSERT_EQ(document.vouchers.size(), 1u);
  auto const& voucher = document.vouchers[0];
  EXPECT_EQ(voucher.series, "A");
  EXPECT_EQ(voucher.number, "1");
  EXPECT_EQ(voucher.date, year_month_day(year{2026}, May, day{5}));
  EXPECT_EQ(voucher.text, "Kassa - PG");
  EXPECT_EQ(voucher.registered, year_month_day(year{2026}, July, day{15}));
  EXPECT_EQ(voucher.signature, "KO");
  EXPECT_EQ(voucher.line, 1u);
  ASSERT_EQ(voucher.transactions.size(), 2u);
  EXPECT_EQ(voucher.transactions[0].amount, -1300000);
  EXPECT_FALSE(voucher.transactions[0].date.has_value());
  auto const& second = voucher.transactions[1];
  EXPECT_EQ(second.objects, (cratchit::sie::Objects{{"1", "100"}}));
  EXPECT_EQ(second.date, year_month_day(year{2026}, May, day{6}));
  EXPECT_EQ(second.text, "In");
  EXPECT_EQ(second.quantity, "2");
  EXPECT_EQ(second.signature, "Kim");
}

TEST(SieDocument, AddedRowSkipsItsDuplicateAndRemovedRowIsKept) {
  // As in the SIE 4B spec and Fortnox: #RTRANS is followed by a like #TRANS (without the signature)
  auto const document = parse_sie("#VER A 36 20251231 \"Ränta\"\n{\n"
                                  "#TRANS 1920 {} 328.68\n"
                                  "#TRANS 8310 {} -328.68\n"
                                  "#RTRANS 1920 {} -328.68 \"\" \"Fel konto\" 0 \"Kim\"\n"
                                  "#TRANS 1920 {} -328.68 \"\" \"Fel konto\" 0\n"
                                  "#RTRANS 1930 {} 328.68 \"\" \"\" 0 \"Kim\"\n"
                                  "#TRANS 1930 {} 328.68\n"
                                  "#BTRANS 6110 {} 800 \"\" \"\" 0 \"Kim\"\n"
                                  "}\n");
  ASSERT_EQ(document.vouchers.size(), 1u);
  auto const& rows = document.vouchers[0].transactions;
  ASSERT_EQ(rows.size(), 5u);
  EXPECT_EQ(rows[2].kind, Transaction::Kind::added);
  EXPECT_EQ(rows[2].signature, "Kim");
  EXPECT_EQ(rows[3].kind, Transaction::Kind::added);
  EXPECT_EQ(rows[3].account, "1930");
  EXPECT_EQ(rows[4].kind, Transaction::Kind::removed);
  EXPECT_EQ(rows[4].account, "6110");
}

TEST(SieDocument, TransAfterRtransThatDiffersIsKept) {
  auto const document = parse_sie("#VER A 1 20250101 \"\"\n{\n#RTRANS 1930 {} 10\n#TRANS 1930 {} 11\n}\n");
  ASSERT_EQ(document.vouchers[0].transactions.size(), 2u);
  EXPECT_EQ(document.vouchers[0].transactions[1].kind, Transaction::Kind::normal);
}

TEST(SieDocument, EmptyVoucher) {
  auto const document = parse_sie("#VER \"\" \"\" 20081216 \"Porto\"\n{\n}\n");
  ASSERT_EQ(document.vouchers.size(), 1u);
  EXPECT_TRUE(document.vouchers[0].series.empty());
  EXPECT_TRUE(document.vouchers[0].transactions.empty());
}

TEST(SieDocument, UnknownLabelsAreCountedAndIgnored) {
  auto const document = parse_sie("#FOO 1\n#FOO 2\n#VER A 1 20250101 \"\"\n{\n#BAR x\n#TRANS 1930 {} 0\n}\n");
  EXPECT_EQ(document.unknown_labels.at("FOO"), 2u);
  EXPECT_EQ(document.unknown_labels.at("BAR"), 1u);
  EXPECT_EQ(document.vouchers[0].transactions.size(), 1u);
}

TEST(SieDocument, ExtraFieldsAreIgnored) {
  auto const document = parse_sie("#KONTO 1930 \"Bank\" extra\n");
  EXPECT_EQ(document.accounts[0].name, "Bank");
}

TEST(SieDocument, FieldErrors) {
  EXPECT_EQ(parse_errors("#RAR 0 20250101 2025-12-31\n"),
            std::vector<std::string>{"line 1: #RAR: end date '2025-12-31' is not a date YYYYMMDD"});
  EXPECT_EQ(parse_errors("#RAR x 20250101 20251231\n"),
            std::vector<std::string>{"line 1: #RAR: year number 'x' is not a number"});
  EXPECT_EQ(parse_errors("#IB 0 1930 1,50\n"), std::vector<std::string>{"line 1: #IB: balance '1,50' is not an amount"});
  EXPECT_EQ(parse_errors("#KONTO\n"), std::vector<std::string>{"line 1: #KONTO: no account number"});
  EXPECT_EQ(parse_errors("#FNAMN {}\n"), std::vector<std::string>{"line 1: #FNAMN: field 1 is an object list, not text"});
  EXPECT_EQ(parse_errors("#VER A 1 20250101\n{\n#TRANS 1930 0 5\n}\n"),
            std::vector<std::string>{"line 3: #TRANS: field 2 is not an object list {...}"});
  EXPECT_EQ(parse_errors("#VER A 1 20250101\n{\n#TRANS 1930 {1} 5\n}\n"),
            std::vector<std::string>{"line 3: #TRANS: an object list must hold pairs of dimension and object"});
}

TEST(SieDocument, StructureErrors) {
  EXPECT_EQ(parse_errors("#TRANS 1930 {} 5\n"),
            std::vector<std::string>{"line 1: #TRANS outside a voucher's { } block"});
  EXPECT_EQ(parse_errors("{\n"), std::vector<std::string>{"line 1: { without a #VER before it"});
  EXPECT_EQ(parse_errors("}\n"), std::vector<std::string>{"line 1: } without a { before it"});
  EXPECT_EQ(parse_errors("#VER A 1 20250101\n#KONTO 1930 x\n"),
            std::vector<std::string>{"line 1: #VER: no { } block with its transactions"});
  EXPECT_EQ(parse_errors("#VER A 1 20250101\n{\n#KONTO 1930 x\n}\n"),
            std::vector<std::string>{"line 3: #KONTO inside a voucher's block"});
  EXPECT_EQ(parse_errors("#VER A 1 20250101\n{\n#TRANS 1930 {} 5\n"),
            std::vector<std::string>{"line 1: #VER: its { } block is not closed"});
  EXPECT_EQ(parse_errors("#X \"open\n"), std::vector<std::string>{"line 1: quote not closed"});
}

TEST(SieChecksum, Crc32IsTheStandardOne) {
  EXPECT_EQ(crc_of("123456789"), 0xCBF43926u);
}

TEST(SieChecksum, CoversLabelsAndFieldsInCp437) {
  // Labels with # and field texts, without blanks between fields, quotes, braces or the { } block lines
  auto const document = parse_sie("#FLAGGA 0\n#KSUMMA\n#KONTO 1915 \"Kassa \\\"special\\\"\"\n"
                                  "#VER A 1 20250101\n{\n#TRANS 1915 {1 \"2\"} 5\n}\n#KSUMMA 1\n");
  EXPECT_TRUE(document.checksum.announced);
  EXPECT_EQ(document.checksum.given, 1u);
  EXPECT_EQ(document.checksum.computed, crc_of("#KONTO1915Kassa \"special\"#VERA120250101#TRANS1915125"));
}

TEST(SieChecksum, TheSpecsExample) {
  // SIE 4B 10.15 underlines #KONTO, 1915, `Kassa `, `"special` and `"`: not the backslashes or outer quotes
  auto const document = parse_sie("#KSUMMA\n#KONTO 1915 \"Kassa \\\"special\\\"\"\n#KSUMMA 0\n");
  EXPECT_EQ(document.checksum.computed, crc_of("#KONTO1915Kassa \"special\""));
}

TEST(SieChecksum, SwedishLettersCountAsCp437Bytes) {
  auto const document = parse_sie("#KSUMMA\n#FNAMN \"\x94\"\n#KSUMMA 0\n");
  EXPECT_EQ(document.checksum.computed, crc_of("#FNAMN\x94"));
}

TEST(SieChecksum, NegativeValueIsTheSameBits) {
  auto const document = parse_sie("#KSUMMA\n#KSUMMA -1\n");
  EXPECT_EQ(document.checksum.given, 0xFFFFFFFFu);
}

TEST(SieChecksum, Errors) {
  EXPECT_EQ(parse_errors("#KSUMMA\n#FLAGGA 0\n"),
            std::vector<std::string>{"#KSUMMA: the file announces a checksum but does not end with one"});
  EXPECT_EQ(parse_errors("#KSUMMA 5\n"),
            std::vector<std::string>{"line 1: #KSUMMA: a checksum without the opening #KSUMMA"});
  EXPECT_EQ(parse_errors("#KSUMMA\n#KSUMMA abc\n"),
            std::vector<std::string>{"line 2: #KSUMMA: 'abc' is not a checksum"});
}

TEST(SieChecksum, NoneByDefault) {
  auto const document = parse_sie("#FLAGGA 0\n");
  EXPECT_FALSE(document.checksum.announced);
  EXPECT_FALSE(document.checksum.given.has_value());
}
