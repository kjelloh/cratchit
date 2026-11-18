#pragma once

#include "core/books/books.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The reports of one fiscal year of the books (index into books.years), as
// lines of text in Swedish, headed by the report, the year and the company.
// From take 3 (plaiground session 60cae9fe, core/books/report), where the
// balance and result accounts were one trial balance (saldobalans).
namespace cratchit::books {

// Balansrapport: each balance account with opening balance (IB), the year's
// change and closing balance (UB), debit positive and credit negative, under
// Tillgångar (class 1) and Eget kapital och skulder (class 2), with their sums.
// Then Beräknat resultat (the result accounts' total, a profit negative) and
// the sum of it all, zero when the books add up. Says where the opening
// balances come from, and lists where carried ones differ from those the
// source reported.
std::vector<std::string> balance_report(Books const& books, std::size_t year, std::string_view company);

// Resultatrapport: each result account with the year's amount, income positive
// and costs negative, in groups by BAS class (3, 4, 5–6, 7, 8) with their sums,
// and Årets resultat (a profit positive). The accounts 8990–8999 (where the
// year-end closing books the result) come after Årets resultat, with what is
// left once they are counted (zero once the result is booked).
std::vector<std::string> result_report(Books const& books, std::size_t year, std::string_view company);

// Verifikationslista: every voucher in book order with its rows in Debet and
// Kredit columns and their sums; rows struck or added after registration are
// marked (a struck row does not count in the sums).
std::vector<std::string> voucher_list(Books const& books, std::size_t year, std::string_view company);

// Huvudbok for one account: opening balance, every row on the account by date
// in Debet and Kredit columns with the balance after it, closing balance.
// Nothing for an account the books neither define nor use in the year.
std::optional<std::vector<std::string>> account_ledger(Books const& books, std::size_t year, std::string_view company,
                                                       std::string_view account);

} // namespace cratchit::books
