#pragma once

#include "core/books/books.hpp"

#include <cstddef>
#include <string>
#include <vector>

// Earlier vouchers as templates for a draft (beslut 2026-11-18, as take 3's
// templates (60cae9fe, task 31 and 41), with the user's rules on accounts and signs)
namespace cratchit {

// A voucher that may serve as the template of a draft
struct Template {
  books::Voucher voucher;
  std::vector<std::string> words; // the words its text shares with the draft's (see shared_words)
};

// The vouchers of books like draft, at most max of them, the best first:
// * a draft with postings: a voucher with a posting on each of their accounts,
//   with the same sign;
// * a draft without postings: a voucher whose text shares words with the
//   draft's heading.
// A draft whose amount is below zero (the other way than usual) is compared
// with its signs turned, as its template is used turned. Only vouchers with an
// amount and at least two postings (not removed) count. The best: more shared
// words, then the latest date; one voucher per pattern of accounts and sides.
std::vector<Template> find_templates(books::Books const& books, books::Draft const& draft, std::size_t max = 9);

// What using a template gives: the draft and the template's accounts left out
// as they are not in the chart of accounts
struct Applied {
  books::Draft draft;
  std::vector<std::string> left_out;
};

// draft with the rows of voucher added: each posting (not removed) on an
// account the draft has no posting on, scaled by the draft's amount over the
// voucher's (rounded to the öre half away from zero), turned when the draft's
// amount is below zero; the last row added gets what remains, so that the
// postings balance. The draft's amount is its event's, else what its parts
// amount to, which becomes the event's amount. The draft's own postings stay;
// it gets the voucher's series, but not its text. Accounts not in accounts are
// left out.
Applied apply_template(books::Draft const& draft, books::Voucher const& voucher,
                       std::vector<books::Account> const& accounts);

} // namespace cratchit
