#include "core/skv/vat_return/clearing.hpp"

#include "core/date.hpp"

#include <algorithm>
#include <format>
#include <string>

namespace cratchit::skv::vat_return {

std::optional<Clearing> clearing_of(Document const& document) {
  books::Postings postings;
  Amount sum = 0;
  auto const add = [&](std::string_view account, Amount amount) {
    postings.push_back({.account = std::string{account}, .amount = amount});
    sum += amount;
  };

  for (auto const& box : document.boxes) {
    if (std::ranges::find(vat_boxes, box.box) == vat_boxes.end()) {
      continue;
    }
    for (auto const& account : box.accounts) {
      if (account.amount != 0) {
        add(account.account, -account.amount);
      }
    }
  }
  if (postings.empty()) {
    return std::nullopt;
  }

  // The form's figure is positive to pay; the books' sign is the other way round
  auto const to_pay = vat_to_pay(document);
  if (to_pay > 0) {
    add(vat_liability_account, -to_pay);
  } else if (to_pay < 0) {
    add(vat_receivable_account, -to_pay);
  }
  if (sum != 0) {
    add(rounding_account, -sum);
  }

  auto const& period = document.period;
  books::Event event{std::format("Momsrapport {} - {}", format_iso_date(period.first), format_iso_date(period.last)),
                     books::debit(postings), period.last};
  return Clearing{std::move(event), std::move(postings)};
}

} // namespace cratchit::skv::vat_return
