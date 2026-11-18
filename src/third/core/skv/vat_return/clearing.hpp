#pragma once

#include "core/books/books.hpp"
#include "core/skv/vat_return/document.hpp"

#include <optional>
#include <string_view>

// The voucher that clears the VAT of a period (Momsrapport): it empties the
// VAT accounts of the period's changes into the VAT clearing account, as box 49
// of the form says, and books what the form's whole kronor leave on the rounding account
namespace cratchit::skv::vat_return {

// Öres- och kronutjämning: takes what is left when box 49 (whole kronor) and the
// VAT accounts (öre and all) do not add up
inline constexpr std::string_view rounding_account = "3740";

// The series a clearing voucher is booked in: M Momsrapport (Fortnox's default series). Fixed for now.
inline constexpr std::string_view clearing_series = "M";

// A voucher candidate, not yet in the books: no series or number
struct Clearing {
  books::Event event;
  books::Postings postings;

  bool operator==(Clearing const&) const = default;
};

// The clearing voucher of the document's period, dated its last day, headed
// "Momsrapport <first> - <last>", its amount the debit total:
// - each account of a VAT box (see vat_boxes) that changed in the period, minus that change
// - box 49 (see vat_to_pay): VAT to pay credits vat_liability_account, VAT to
//   get back debits vat_receivable_account; nothing when it is 0
// - rounding_account with what makes the postings add up to zero, when anything does
// Nothing when no VAT account changed in the period.
std::optional<Clearing> clearing_of(Document const& document);

} // namespace cratchit::skv::vat_return
