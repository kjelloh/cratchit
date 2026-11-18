#pragma once

#include "core/balances.hpp"
#include "core/ink2/form.hpp"
#include "core/ink2/mapping.hpp"
#include "core/ink2/previous.hpp"
#include "core/ink2/skv.hpp"
#include "core/ink2/tax.hpp"
#include "core/sie/document.hpp"

#include <string>
#include <vector>

namespace cratchit::ink2 {

// The return traced from the vouchers up, in the order it is made: each field of INK2R and the
// INK2S fields of the mapping that has an amount, with the mapping's accounts, how the field shows
// them, each account's opening and closing balance (or result) and its vouchers of fiscal year 0,
// and the sum rounded; then the INK2S and INK2 fields made by the rules, with their numbers; then
// the accounts with an amount on no field. balances are those of fiscal year 0 that form was made of.
std::string format_trace(sie::Document const& document, std::vector<Line> const& lines,
                         std::vector<SkvField> const& skv, Balances const& balances, Form const& form,
                         TaxReturn const& tax, Previous const& previous);

} // namespace cratchit::ink2
