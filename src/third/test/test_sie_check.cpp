#include "core/bas.hpp"
#include "core/sie/check.hpp"

#include "sie_test_file.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>

using cratchit::AccountKind;
using cratchit::account_kind;
using cratchit::sie::check_document;
using sie_test::balanced_balances;
using sie_test::parse_sie;
using sie_test::sale_and_cost;
using sie_test::sie_file;

namespace {

bool has(std::vector<std::string> const& texts, std::string const& text) {
  return std::ranges::find(texts, text) != texts.end();
}

} // namespace

TEST(AccountKind, TypeFirstThenBasClass) {
  EXPECT_EQ(account_kind("1930", ""), AccountKind::balance);
  EXPECT_EQ(account_kind("2099", ""), AccountKind::balance);
  EXPECT_EQ(account_kind("3010", ""), AccountKind::result);
  EXPECT_EQ(account_kind("8999", ""), AccountKind::result);
  EXPECT_EQ(account_kind("9021", ""), AccountKind::unknown);
  EXPECT_EQ(account_kind("3010", "T"), AccountKind::balance);
  EXPECT_EQ(account_kind("1930", "K"), AccountKind::result);
  EXPECT_EQ(account_kind("9021", "S"), AccountKind::balance);
  EXPECT_EQ(account_kind("", ""), AccountKind::unknown);
}

TEST(SieCheck, ConsistentFileHasNoFindings) {
  auto const findings = check_document(parse_sie(sie_file(2025, balanced_balances(), sale_and_cost(2025))));
  EXPECT_TRUE(findings.errors.empty()) << findings.errors.front();
  EXPECT_TRUE(findings.notes.empty()) << findings.notes.front();
}

TEST(SieCheck, IdentificationErrors) {
  auto const findings = check_document(parse_sie("#SIETYP 2\n#VALUTA NOK\n#FORMAT PC9\n#FOO x\n"));
  EXPECT_TRUE(has(findings.errors, "Only SIE type 4 (4E) can be imported; this file is type 2"));
  EXPECT_TRUE(has(findings.errors, "No company name (#FNAMN)"));
  EXPECT_TRUE(has(findings.errors, "No organisation number (#ORGNR)"));
  EXPECT_TRUE(has(findings.errors, "Only SEK is supported; the file is in NOK (#VALUTA)"));
  EXPECT_TRUE(has(findings.errors, "No current fiscal year (#RAR 0)"));
  EXPECT_TRUE(has(findings.notes, "#FORMAT is PC9, not PC8"));
  EXPECT_TRUE(has(findings.notes, "Ignored 1 unknown record #FOO"));
}

TEST(SieCheck, VoucherErrors) {
  auto const vouchers = "#VER A 1 20250201 \"x\"\n{\n#TRANS 1930 {} 5\n#TRANS 3010 {} -4\n}\n"
                        "#VER A 1 20250202 \"x\"\n{\n}\n"
                        "#VER A 2 20260101 \"x\"\n{\n#TRANS 1234 {} 0\n}\n"
                        "#VER \"\" 3 20250101 \"x\"\n{\n}\n";
  auto const findings = check_document(parse_sie(sie_file(2025, "", vouchers)));
  EXPECT_TRUE(has(findings.errors, "line 16: voucher A 1 does not balance: its amounts add up to 1.00"));
  EXPECT_TRUE(has(findings.errors, "line 21: voucher A 1 is given twice"));
  EXPECT_TRUE(has(findings.errors,
                  "line 24: voucher A 2 is dated 2026-01-01, outside the fiscal year 2025-01-01 - 2025-12-31"));
  EXPECT_TRUE(has(findings.errors, "line 24: voucher A 2 uses account 1234, which the file does not define (#KONTO)"));
  EXPECT_TRUE(has(findings.errors, "line 28: a voucher without series or number cannot be imported"));
}

TEST(SieCheck, RemovedRowsDoNotCount) {
  auto const vouchers = "#VER A 1 20250201 \"x\"\n{\n#TRANS 1930 {} 5\n#TRANS 3010 {} -5\n#BTRANS 6570 {} 7\n}\n";
  auto const findings = check_document(
      parse_sie(sie_file(2025, "#UB 0 1930 5\n#RES 0 3010 -5\n", vouchers)));
  EXPECT_TRUE(findings.errors.empty()) << findings.errors.front();
  EXPECT_TRUE(findings.notes.empty()) << findings.notes.front();
}

TEST(SieCheck, BalancesThatDoNotAddUpAreNoted) {
  auto const balances = "#UB -1 1930 900\n#IB 0 1930 1000\n#UB 0 1930 1400\n#RES 0 3010 -400\n#RES 0 6570 10\n";
  auto const findings = check_document(parse_sie(sie_file(2025, balances, sale_and_cost(2025))));
  EXPECT_TRUE(findings.errors.empty());
  EXPECT_TRUE(has(findings.notes, "In the file, 1930 Bank: opening balance 1000.00 + transactions 490.00 = 1490.00, "
                                  "but the closing balance (#UB) is 1400.00"));
  EXPECT_TRUE(has(findings.notes, "In the file, 3010 Sales: transactions -500.00, but the result (#RES) is -400.00"));
  EXPECT_TRUE(has(findings.notes, "In the file, 1930 Bank: opening balance 1000.00 differs from the year before's "
                                  "closing balance 900.00 (difference 100.00)"));
  EXPECT_EQ(findings.notes.size(), 3u);
}

TEST(SieCheck, ChecksumMismatchIsAnError) {
  auto const findings = check_document(parse_sie("#KSUMMA\n#FLAGGA 0\n#KSUMMA 1\n"));
  auto const it = std::ranges::find_if(findings.errors, [](std::string const& e) {
    return e.starts_with("The checksum #KSUMMA 1 does not match the file's content (");
  });
  EXPECT_NE(it, findings.errors.end());
}
