#pragma once

#include "core/sie/document.hpp"
#include "core/sie/file.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace sie_test {

// A small SIE 4E file of Test AB (556000-0001) for calendar year `year` and
// the year before, with the given balance records and vouchers (whole lines)
inline std::string sie_file(int year, std::string_view balances, std::string_view vouchers,
                            std::string_view company = "\"Test AB\"", std::string_view orgnr = "556000-0001") {
  return std::format("#FLAGGA 0\r\n"
                     "#FORMAT PC8\r\n"
                     "#SIETYP 4\r\n"
                     "#PROGRAM \"Test\" 1.0\r\n"
                     "#GEN 20261019\r\n"
                     "#FNAMN {2}\r\n"
                     "#ORGNR {3}\r\n"
                     "#RAR 0 {0}0101 {0}1231\r\n"
                     "#RAR -1 {1}0101 {1}1231\r\n"
                     "#KPTYP EUBAS97\r\n"
                     "#KONTO 1930 \"Bank\"\r\n"
                     "#SRU 1930 7281\r\n"
                     "#KONTO 2099 \"Result\"\r\n"
                     "#KONTO 3010 \"Sales\"\r\n"
                     "#KONTO 6570 \"Bank costs\"\r\n"
                     "{4}"
                     "{5}",
                     year, year - 1, company, orgnr, balances, vouchers);
}

// Balances that add up for vouchers selling 500 (A 1) and paying 10 in bank costs (A 2)
inline std::string balanced_balances() {
  return "#IB -1 1930 0\r\n"
         "#UB -1 1930 1000.00\r\n"
         "#UB -1 2099 -1000.00\r\n"
         "#IB 0 1930 1000.00\r\n"
         "#IB 0 2099 -1000.00\r\n"
         "#UB 0 1930 1490.00\r\n"
         "#UB 0 2099 -1000.00\r\n"
         "#RES 0 3010 -500\r\n"
         "#RES 0 6570 10\r\n";
}

inline std::string sale_and_cost(int year) {
  return std::format("#VER A 1 {0}0201 \"Sale\" {0}0205\r\n"
                     "{{\r\n"
                     "#TRANS 1930 {{}} 500.00\r\n"
                     "#TRANS 3010 {{}} -500.00\r\n"
                     "}}\r\n"
                     "#VER A 2 {0}0301 \"Bank cost\"\r\n"
                     "{{\r\n"
                     "#TRANS 6570 {{}} 10\r\n"
                     "#TRANS 1930 {{}} -10\r\n"
                     "}}\r\n",
                     year);
}

inline std::vector<std::uint8_t> as_bytes(std::string_view text) {
  return {text.begin(), text.end()};
}

// The document of an SIE text in CP437 (ASCII in tests), which must lex and parse without errors
inline cratchit::sie::Document parse_sie(std::string_view text) {
  auto const lexed = cratchit::sie::lex_bytes(as_bytes(text));
  EXPECT_TRUE(lexed.has_value()) << lexed.error();
  if (!lexed) {
    return {};
  }
  auto parsed = cratchit::sie::parse_document(lexed->lines);
  for (auto const& error : parsed.errors) {
    ADD_FAILURE() << error;
  }
  return std::move(parsed.document);
}

// The errors of parsing an SIE text
inline std::vector<std::string> parse_errors(std::string_view text) {
  auto const lexed = cratchit::sie::lex_bytes(as_bytes(text));
  EXPECT_TRUE(lexed.has_value());
  return lexed ? cratchit::sie::parse_document(lexed->lines).errors : std::vector<std::string>{};
}

} // namespace sie_test
