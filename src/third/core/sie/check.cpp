#include "core/sie/check.hpp"

#include <algorithm>
#include <format>
#include <map>
#include <set>
#include <utility>

namespace cratchit::sie {

namespace {

// The balances of one year by account
std::map<std::string, Amount> by_account(std::vector<Balance> const& balances, int year) {
  std::map<std::string, Amount> result;
  for (auto const& balance : balances) {
    if (balance.year == year) {
      result[balance.account] += balance.amount;
    }
  }
  return result;
}

Amount value_or_zero(std::map<std::string, Amount> const& amounts, std::string const& account) {
  auto const it = amounts.find(account);
  return it == amounts.end() ? 0 : it->second;
}

std::string account_label(Document const& document, std::string const& number) {
  auto const it = std::ranges::find(document.accounts, number, &Account::number);
  return it == document.accounts.end() || it->name.empty() ? number : std::format("{} {}", number, it->name);
}

} // namespace

Findings check_document(Document const& document) {
  Findings findings;
  auto& errors = findings.errors;
  auto& notes = findings.notes;

  if (document.sie_type != "4") {
    errors.push_back(std::format("Only SIE type 4 (4E) can be imported; this file is type {}",
                                 document.sie_type.empty() ? "unknown (no #SIETYP)" : document.sie_type));
  }
  if (!document.format.empty() && document.format != "PC8") {
    notes.push_back(std::format("#FORMAT is {}, not PC8", document.format));
  }
  if (document.company_name.empty()) {
    errors.push_back("No company name (#FNAMN)");
  }
  if (document.orgnr.empty()) {
    errors.push_back("No organisation number (#ORGNR)");
  }
  if (!document.currency.empty() && document.currency != "SEK") {
    errors.push_back(std::format("Only SEK is supported; the file is in {} (#VALUTA)", document.currency));
  }
  for (auto const& [label, count] : document.unknown_labels) {
    notes.push_back(std::format("Ignored {} unknown record{} #{}", count, count == 1 ? "" : "s", label));
  }
  auto const& checksum = document.checksum;
  if (checksum.given && *checksum.given != checksum.computed) {
    errors.push_back(std::format("The checksum #KSUMMA {} does not match the file's content ({})", *checksum.given,
                                 checksum.computed));
  }

  auto const year = document.year(0);
  if (!year) {
    errors.push_back("No current fiscal year (#RAR 0)");
  }

  std::set<std::string> defined;
  for (auto const& account : document.accounts) {
    defined.insert(account.number);
  }
  std::set<std::pair<std::string, std::string>> seen;
  std::map<std::string, Amount> changes; // per account, from the vouchers
  for (auto const& voucher : document.vouchers) {
    auto const name = std::format("line {}: voucher {} {}", voucher.line, voucher.series, voucher.number);
    if (voucher.series.empty() || voucher.number.empty()) {
      errors.push_back(std::format("line {}: a voucher without series or number cannot be imported", voucher.line));
    } else if (!seen.insert({voucher.series, voucher.number}).second) {
      errors.push_back(std::format("{} is given twice", name));
    }
    if (year && (voucher.date < year->start || voucher.date > year->end)) {
      errors.push_back(std::format("{} is dated {}, outside the fiscal year {} - {}", name,
                                   format_iso_date(voucher.date), format_iso_date(year->start),
                                   format_iso_date(year->end)));
    }
    Amount sum = 0;
    for (auto const& transaction : voucher.transactions) {
      if (!defined.contains(transaction.account)) {
        errors.push_back(std::format("{} uses account {}, which the file does not define (#KONTO)", name,
                                     transaction.account));
      }
      if (transaction.kind != Transaction::Kind::removed) {
        sum += transaction.amount;
        changes[transaction.account] += transaction.amount;
      }
    }
    if (sum != 0) {
      errors.push_back(std::format("{} does not balance: its amounts add up to {}", name, format_amount(sum)));
    }
  }

  auto const opening = by_account(document.opening, 0);
  auto const closing = by_account(document.closing, 0);
  auto const results = by_account(document.results, 0);
  auto const previous_closing = by_account(document.closing, -1);
  auto const& change_totals = changes;
  std::set<std::string> accounts;
  for (auto const* amounts : {&opening, &closing, &results, &change_totals}) {
    for (auto const& [account, amount] : *amounts) {
      accounts.insert(account);
    }
  }
  for (auto const& account : accounts) {
    auto const it = std::ranges::find(document.accounts, account, &Account::number);
    auto const kind = account_kind(account, it == document.accounts.end() ? "" : it->type);
    auto const change = value_or_zero(changes, account);
    auto const label = account_label(document, account);
    if (kind == AccountKind::balance) {
      auto const expected = value_or_zero(opening, account) + change;
      if (expected != value_or_zero(closing, account)) {
        notes.push_back(std::format("In the file, {}: opening balance {} + transactions {} = {}, but the closing "
                                    "balance (#UB) is {}",
                                    label, format_amount(value_or_zero(opening, account)), format_amount(change),
                                    format_amount(expected), format_amount(value_or_zero(closing, account))));
      }
    } else if (kind == AccountKind::result && change != value_or_zero(results, account)) {
      notes.push_back(std::format("In the file, {}: transactions {}, but the result (#RES) is {}", label,
                                  format_amount(change), format_amount(value_or_zero(results, account))));
    }
  }
  if (document.year(-1)) {
    std::set<std::string> carried;
    for (auto const* amounts : {&opening, &previous_closing}) {
      for (auto const& [account, amount] : *amounts) {
        carried.insert(account);
      }
    }
    for (auto const& account : carried) {
      auto const was = value_or_zero(previous_closing, account);
      auto const is = value_or_zero(opening, account);
      if (was != is) {
        notes.push_back(std::format("In the file, {}: opening balance {} differs from the year before's closing "
                                    "balance {} (difference {})",
                                    account_label(document, account), format_amount(is), format_amount(was),
                                    format_amount(is - was)));
      }
    }
  }
  return findings;
}

} // namespace cratchit::sie
