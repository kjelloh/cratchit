#pragma once

#include "core/balances.hpp"
#include "core/date.hpp"
#include "core/ink2/form.hpp"
#include "core/sru.hpp"

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit::ink2 {

// What this year's INK2S takes from last year
struct Previous {
  Kronor deficit = 0;              // last year's 4.16 Underskott (7770): this year's 4.14a (7763)
  std::string source;              // where it is from, for the notes: the file, or e.g. --previous-deficit
  std::vector<std::string> marks;  // the yes/no marks last year's INK2S gave (8040, 8041, 8044, 8045)
  std::optional<SruForm> ink2r;    // last year's INK2R, for checking this year's opening balances
};

// The SRU file of last year's return: path when it is a file, else the file BLANKETTER.SRU
// (in any case) in the folder path. Error text when there is none.
std::expected<std::filesystem::path, std::string> sru_file_in(std::filesystem::path const& path);

// Last year's return from its SRU forms (source names them). Its INK2S must be there, for the
// same company as orgnr, and for the fiscal year ending the day before start (7012). Its 4.16
// Underskott (7770) is the deficit; none when it has 4.15 Överskott (7670) instead.
std::expected<Previous, std::string> previous_from_sru(std::vector<SruForm> const& forms, std::string_view source,
                                                       std::string_view orgnr, Date start);

// The deficit given as a number, e.g. by --previous-deficit
Previous previous_deficit(Kronor deficit, std::string_view source);

// The digits of an orgnr, without the 16 SRU files may put before it: 165567828172 and
// 556782-8172 are both 5567828172
std::string orgnr_digits(std::string_view orgnr);

// The opening balances of fiscal year 0 (#IB)
Balances opening_balances(sie::Document const& document);

// Notes where the balance sheet of the opening balances (opening, made by the mapping) differs
// from last year's filed INK2R, one per field
std::vector<std::string> check_opening(Form const& opening, SruForm const& last_ink2r);

} // namespace cratchit::ink2
