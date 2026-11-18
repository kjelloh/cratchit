#include "core/ink2/trace.hpp"

#include "sie_test_file.hpp"

#include <gtest/gtest.h>

namespace {

std::vector<cratchit::ink2::Line> const& mapping() {
  static auto const lines = *cratchit::ink2::parse_mapping(cratchit::ink2::mapping_text());
  return lines;
}

std::vector<cratchit::ink2::SkvField> const& skv() {
  static auto const fields = *cratchit::ink2::parse_skv_fields(cratchit::ink2::skv_fields_text());
  return fields;
}

// Test AB's year: a sale of 500 and 10 in bank costs (A 1, A 2), closed with a profit of 490
std::string trace_of(cratchit::sie::Document const& document) {
  auto const balances = cratchit::year_balances(document, 0);
  auto const form = cratchit::ink2::build_form(mapping(), balances);
  cratchit::ink2::Previous const previous{100, "last.sru", {"8041"}, std::nullopt};
  auto const tax = cratchit::ink2::build_tax_return(skv(), form, previous);
  return cratchit::ink2::format_trace(document, mapping(), skv(), balances, form, tax, previous);
}

cratchit::sie::Document books() {
  return sie_test::parse_sie(sie_test::sie_file(2025,
                                                "#IB 0 1930 1000.00\r\n"
                                                "#IB 0 2099 -1000.00\r\n"
                                                "#UB 0 1930 1490.00\r\n"
                                                "#UB 0 2099 -1490.00\r\n"
                                                "#RES 0 3010 -500\r\n"
                                                "#RES 0 6570 10\r\n"
                                                "#RES 0 8999 490\r\n",
                                                sie_test::sale_and_cost(2025)));
}

} // namespace

TEST(Ink2Trace, AccountsAndVouchersOfAField) {
  auto const trace = trace_of(books());
  EXPECT_NE(trace.find("INK2R 2.26  7281  Kassa, bank och redovisningsmedel\n"
                       "  Mapping: 19xx; shown as booked\n"
                       "  1930 Bank   IB 1000.00   UB 1490.00\n"
                       "      2025-02-01 A 1      Sale                                                     500.00\n"
                       "      2025-03-01 A 2      Bank cost                                                -10.00\n"
                       "  Sum 1490.00 as booked, 1490.00 shown: 1 490 kr\n"),
            std::string::npos)
      << trace;
  EXPECT_NE(trace.find("INK2R 3.1  7410  Nettoomsättning\n"
                       "  Mapping: 30xx-37xx; shown negated\n"
                       "  3010 Sales   RES -500.00\n"
                       "      2025-02-01 A 1      Sale                                                    -500.00\n"
                       "  Sum -500.00 as booked, 500.00 shown: 500 kr\n"),
            std::string::npos)
      << trace;
}

TEST(Ink2Trace, ConditionsAndRules) {
  auto const trace = trace_of(books());
  EXPECT_NE(trace.find("  + 899x (vinst): the accounts of the group net 490.00 (a debit), so this field\n"), std::string::npos)
      << trace;
  EXPECT_NE(trace.find("INK2S 4.1  7650  490\n  Årets resultat, vinst = INK2R 3.26 (7450)\n"), std::string::npos);
  EXPECT_NE(trace.find("INK2S 4.14a  7763  100\n"), std::string::npos);
  EXPECT_NE(trace.find("INK2S 4.15  7670  390\n  SKV's + fields less its - fields: + 4.1 490 - 4.14a 100 = 390, so "
                       "4.15 Överskott 390\n"),
            std::string::npos)
      << trace;
  EXPECT_NE(trace.find("INK2 1.1  7104  390\n"), std::string::npos);
  EXPECT_NE(trace.find("INK2S marks 8041: X, as last year's return (last.sru)\n"), std::string::npos);
  EXPECT_NE(trace.find("Accounts with an amount on no field of INK2R\n"
                       "============================================\n"
                       "None.\n"),
            std::string::npos);
}

TEST(Ink2Trace, AccountsOnNoField) {
  auto document = books();
  document.results.push_back({0, "9021", 1000, ""});
  auto const trace = trace_of(document);
  EXPECT_NE(trace.find("============================================\n  9021    RES 10.00\n"), std::string::npos)
      << trace;
}
