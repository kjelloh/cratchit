#pragma once

#include "core/amount.hpp"
#include "core/balances.hpp"
#include "core/ink2/mapping.hpp"

#include <map>
#include <string>
#include <vector>

namespace cratchit::ink2 {

// The amount of a field of the mapping
struct FieldAmount {
  bool used = false; // some account with an amount is in it; then shown even when 0
  Amount exact = 0;  // in öre, as the field shows it (see Field::sign)
  Kronor kronor = 0; // exact, rounded half away from zero
  Balances accounts; // the accounts in it with their amounts as booked
};

// INK2R (räkenskapsschema) of one fiscal year, and the INK2S fields that come from accounts
struct Form {
  std::map<std::string, FieldAmount> fields; // by SRU code, every field of the mapping
  // Accounts of class 1-8 with an amount that go to no field, or to more than one
  std::vector<std::string> errors;
  // Accounts that are not of class 1-8 (not on INK2R), and what does not add up in whole kronor
  std::vector<std::string> notes;
};

// The SRU codes the books give their accounts (SIE #SRU), by account
using SruCodes = std::map<std::string, std::string>;

SruCodes sru_codes(sie::Document const& document);

// The fields of lines for the balances of a fiscal year. Each account with an amount goes to
// the fields with a group of accounts that holds it, whose condition the group meets: "Om
// netto +" (and the profit of "+ 899x") when the group of the account nets to a credit (or a
// debit for 899x) or zero, "Om netto -" (the loss of "– 899x") when to the other side.
//
// The books' #SRU codes (sru) are checked against it, and decide where the mapping does not:
// - In two fields or more (BAS lists 40xx-47xx under both 3.5 and 3.6): the one of them its #SRU
//   names, with a note; an error when it names none of them.
// - In no field: the field its #SRU names, with a note; an error when it names no field of lines.
// - In one field, and its #SRU names another: a note, the mapping decides. A #SRU naming the other
//   field of a plus/minus pair the account is in (e.g. 8999 -> 7450 for a loss) agrees.
// - A note lists the accounts with an amount that have no #SRU, when the books give any.
//
// All of this is for INK2R. An INK2S field of the mapping (e.g. 4.3c, costs not to deduct)
// adds up the accounts it holds, which are also in their INK2R field.
Form build_form(std::vector<Line> const& lines, Balances const& balances, SruCodes const& sru = {});

// The sums of the form in whole kronor, as the fields show them
struct Sums {
  Kronor assets = 0;            // 72xx
  Kronor equity_and_debt = 0;   // 73xx
  Kronor result = 0;            // 74xx less 75xx, but 7450 and 7550: income less costs
  Kronor year_result = 0;       // 7450 less 7550, as 3.26 and 3.27 give it
};

Sums sums_of(Form const& form);

} // namespace cratchit::ink2
