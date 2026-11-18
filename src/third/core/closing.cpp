#include "core/closing.hpp"

#include <algorithm>
#include <format>

namespace cratchit {

namespace {

bool is_account(std::string_view account) {
  return account.size() == 4 && std::ranges::all_of(account, [](char c) { return c >= '0' && c <= '9'; });
}

// The sum of the accounts first to last, four digit numbers compared as text
Amount sum_of(Balances const& balances, std::string_view first, std::string_view last) {
  Amount sum = 0;
  for (auto const& [account, amount] : balances) {
    if (is_account(account) && first <= account && account <= last) {
      sum += amount;
    }
  }
  return sum;
}

} // namespace

std::vector<std::string> check_closing(Balances const& balances, Date balance_day) {
  std::vector<std::string> notes;
  auto const day = format_iso_date(balance_day);
  if (auto const vat = sum_of(balances, "2610", "2659"); vat > 0) {
    notes.push_back(std::format("Moms (2610-2659) nets to a receivable {} at {}; BAS: move it to 1650 Momsfordran",
                                format_amount(vat), day));
  }
  if (auto const tax = sum_of(balances, "2500", "2599"); tax > 0) {
    notes.push_back(std::format(
        "Skatteskulder (2500-2599) net to a receivable {} at {}; BAS: move it to 1640 Skattefordringar",
        format_amount(tax), day));
  }
  if (auto const tax_account = sum_of(balances, "1630", "1630"); tax_account < 0) {
    notes.push_back(std::format("1630 Skattekonto has a credit balance {} at {}; BAS: move it to 2850",
                                format_amount(tax_account), day));
  }
  auto const held = sum_of(balances, "1990", "1999");
  if (auto const owed = sum_of(balances, "2830", "2839"); held != 0 && held + owed != 0) {
    notes.push_back(std::format("Redovisningsmedel (1990-1999) {} at {} is not matched by Avräkning för annans "
                                "räkning (2830-2839) {}; BAS 1990 is money held for others, not a suspense account",
                                format_amount(held), day, format_amount(-owed)));
  }
  bool const depreciates = sum_of(balances, "7800", "7899") != 0;
  for (auto const& [account, amount] : balances) {
    bool const accumulated_depreciation = is_account(account) && account < "1300" && account.ends_with('9');
    if (!depreciates && accumulated_depreciation && amount != 0) {
      notes.push_back(std::format("No depreciation (7800-7899) in the year ending {}, although {} (accumulated "
                                  "depreciation) is {}; depreciation is a closing entry",
                                  day, account, format_amount(amount)));
    }
  }
  if (auto const last_year = sum_of(balances, "2098", "2098"); last_year != 0) {
    notes.push_back(std::format("2098 Vinst eller förlust från föregående år is {} at {}; last year's result is not "
                                "yet moved on as the årsstämma decided (e.g. to 2091 Balanserat resultat)",
                                format_amount(last_year), day));
  }
  if (auto const suspense = sum_of(balances, "2999", "2999"); suspense != 0) {
    notes.push_back(std::format("2999 OBS-konto is {} at {}; BAS: it must be zero at year end", format_amount(suspense), day));
  }
  return notes;
}

std::optional<Amount> unbooked_result(sie::Document const& document) {
  Amount results = 0;
  Amount closing_posts = 0;
  for (auto const& result : document.results) {
    if (result.year != 0 || !is_account(result.account)) {
      continue;
    }
    if ("8990" <= result.account && result.account <= "8999") {
      closing_posts += result.amount;
    } else if ("3000" <= result.account && result.account <= "8989") {
      results += result.amount;
    }
  }
  Amount balances = 0;
  for (auto const& balance : document.closing) {
    if (balance.year == 0 && is_account(balance.account) && balance.account < "3000") {
      balances += balance.amount;
    }
  }
  // Closed books: balances add up to 0. Unclosed: they lack exactly the result that is not on 2099.
  if (closing_posts != 0 || results == 0 || balances + results != 0) {
    return std::nullopt;
  }
  return results;
}

sie::Document with_closing_entry(sie::Document document, Amount result) {
  auto const year = document.year(0);
  auto const day = year ? year->end : Date{};
  sie::Voucher voucher{"*", "", day, std::string(simulated_closing_text), std::nullopt, "", {}, 0};
  sie::Transaction to_equity;
  to_equity.account = "2099";
  to_equity.amount = result;
  sie::Transaction from_result;
  from_result.account = "8999";
  from_result.amount = -result;
  voucher.transactions = {to_equity, from_result};
  document.vouchers.push_back(std::move(voucher));
  auto const closing = std::ranges::find_if(document.closing, [](sie::Balance const& balance) {
    return balance.year == 0 && balance.account == "2099";
  });
  if (closing == document.closing.end()) {
    document.closing.push_back({0, "2099", result, ""});
  } else {
    closing->amount += result;
  }
  document.results.push_back({0, "8999", -result, ""});
  return document;
}

} // namespace cratchit
