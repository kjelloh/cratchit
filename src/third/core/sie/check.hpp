#pragma once

#include "core/bas.hpp"
#include "core/sie/document.hpp"

#include <string>
#include <vector>

namespace cratchit::sie {

// What is wrong with a file (it must not be imported) and what is worth noting
struct Findings {
  std::vector<std::string> errors;
  std::vector<std::string> notes;
};

// Checks a parsed SIE 4E file on its own, without the books it may go into.
// Errors: not SIE type 4; no fiscal year 0, #FNAMN or #ORGNR; a currency other
// than SEK; a voucher without series or number, given twice, dated outside year
// 0, not balancing, or using an account the file does not define; a checksum
// that does not match. Notes: a #FORMAT other than PC8, ignored unknown labels,
// and year 0 balances that do not add up: opening + transactions against the
// closing balance (balance accounts), transactions against the result (result
// accounts), and the opening balance against the year before's closing balance.
Findings check_document(Document const& document);

} // namespace cratchit::sie
