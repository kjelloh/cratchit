#include "core/books/report.hpp"

#include "core/amount.hpp"
#include "core/date.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <map>
#include <set>

namespace cratchit::books {

namespace {

// The width of UTF-8 text in the terminal, counted in code points
std::size_t width_of(std::string_view text) {
  return static_cast<std::size_t>(
      std::ranges::count_if(text, [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; }));
}

// text padded with spaces after it, or before it, to width
std::string left(std::string_view text, std::size_t width) {
  auto const used = width_of(text);
  return std::string(text) + std::string(used < width ? width - used : 0, ' ');
}
std::string right(std::string_view text, std::size_t width) {
  auto const used = width_of(text);
  return std::string(used < width ? width - used : 0, ' ') + std::string(text);
}

// rest after two spaces, or nothing (and no trailing spaces) when it is empty
std::string with_rest(std::string line, std::string_view rest) {
  if (!rest.empty()) return std::format("{}  {}", line, rest);
  line.erase(line.find_last_not_of(' ') + 1);
  return line;
}

std::string year_text(FiscalYear const& year) {
  return std::format("{} – {}", format_iso_date(year.start), format_iso_date(year.end));
}

std::string account_name(Books const& books, std::string_view number) {
  auto const* account = books.account(number);
  return account ? account->name : std::string{};
}

Amount value_or_zero(std::map<std::string, Amount> const& amounts, std::string const& account) {
  auto const it = amounts.find(account);
  return it == amounts.end() ? 0 : it->second;
}

// Account numbers in number order (shorter first)
bool number_less(std::string const& a, std::string const& b) {
  return a.size() != b.size() ? a.size() < b.size() : a < b;
}
using Accounts = std::set<std::string, decltype(&number_less)>;

std::vector<std::string> heading(std::string_view title, FiscalYear const& year, std::string_view company) {
  return {std::format("{} {}", title, year_text(year)), std::string(company), ""};
}

// The balance report's columns: account, IB, change, UB, then the name
constexpr std::size_t account_width = 8;
constexpr std::size_t amount_width = 15;
std::string balance_row(std::string_view first, std::string_view opening, std::string_view change,
                        std::string_view closing, std::string_view rest) {
  return with_rest(left(first, account_width) + right(opening, amount_width) + right(change, amount_width) +
                       right(closing, amount_width),
                   rest);
}

// The result report's columns: account, amount, then the name
std::string result_row(std::string_view first, std::string_view amount, std::string_view rest) {
  return with_rest(left(first, account_width) + right(amount, amount_width), rest);
}

// The voucher list's posting columns: account, debit, credit, then the text.
// The amount without sign on its side, the other side empty.
struct Sides {
  std::string debit;
  std::string credit;
};
Sides sides_of(Amount amount) {
  if (amount < 0) return {"", format_amount(-amount)};
  return {format_amount(amount), ""};
}
std::string posting_row(std::string_view first, Sides const& sides, std::string_view rest) {
  return with_rest(left(first, 7) + right(sides.debit, 13) + right(sides.credit, 13), rest);
}

// The year's accounts with an amount in any of amounts, in number order
Accounts accounts_in(std::initializer_list<std::map<std::string, Amount> const*> amounts) {
  Accounts accounts(&number_less);
  for (auto const* each : amounts) {
    for (auto const& [account, amount] : *each) accounts.insert(account);
  }
  return accounts;
}

// The result accounts where the year-end closing books the year's result
bool is_result_booking(std::string_view account) {
  return account.size() == 4 && account.starts_with("899");
}

// A group of the result report: the BAS classes (first digit) it holds
struct ResultGroup {
  std::string_view classes;
  std::string_view heading;
  std::string_view sum;
};
constexpr std::array result_groups{
    ResultGroup{"3", "Rörelsens intäkter", "Summa rörelsens intäkter"},
    ResultGroup{"4", "Varor, material och vissa köpta tjänster", "Summa varor, material och tjänster"},
    ResultGroup{"56", "Övriga externa kostnader", "Summa övriga externa kostnader"},
    ResultGroup{"7", "Personal, avskrivningar m.m.", "Summa personal, avskrivningar m.m."},
    ResultGroup{"8", "Finansiella och andra intäkter och kostnader", "Summa finansiella och andra poster"},
    ResultGroup{"", "Övriga resultatkonton", "Summa övriga resultatkonton"}, // any other first digit
};
std::size_t result_group_of(std::string_view account) {
  for (std::size_t i = 0; i + 1 < result_groups.size(); ++i) {
    if (!account.empty() && result_groups[i].classes.find(account.front()) != std::string_view::npos) return i;
  }
  return result_groups.size() - 1;
}

} // namespace

std::vector<std::string> balance_report(Books const& books, std::size_t year_index, std::string_view company) {
  auto const& year = books.years[year_index];
  auto const all = balances(books);
  auto const& b = all[year_index];
  auto lines = heading("Balansrapport", year, company);
  lines.push_back(balance_row("Konto", "IB", "Förändring", "UB", "Namn"));

  auto const accounts = accounts_in({&b.opening, &b.change, &b.closing});
  struct Sums {
    Amount opening = 0;
    Amount change = 0;
    Amount closing = 0;
  };
  Sums total;
  // Tillgångar (class 1), Eget kapital och skulder (class 2), and any other balance account
  auto group = [&](std::string_view sum_text, auto&& belongs) {
    Sums sums;
    std::vector<std::string> rows;
    for (auto const& account : accounts) {
      if (account_kind(books, account) != AccountKind::balance || !belongs(account)) continue;
      auto const opening = value_or_zero(b.opening, account);
      auto const change = value_or_zero(b.change, account);
      auto const closing = value_or_zero(b.closing, account);
      if (opening == 0 && change == 0 && closing == 0) continue;
      sums.opening += opening;
      sums.change += change;
      sums.closing += closing;
      rows.push_back(balance_row(account, format_amount(opening), format_amount(change), format_amount(closing),
                                 account_name(books, account)));
    }
    total.opening += sums.opening;
    total.change += sums.change;
    total.closing += sums.closing;
    return std::pair{std::move(rows), balance_row("", format_amount(sums.opening), format_amount(sums.change),
                                                  format_amount(sums.closing), sum_text)};
  };
  auto const add = [&](std::string_view title, std::pair<std::vector<std::string>, std::string> const& rows_and_sum) {
    lines.push_back("");
    lines.push_back(std::string(title));
    lines.insert(lines.end(), rows_and_sum.first.begin(), rows_and_sum.first.end());
    lines.push_back(rows_and_sum.second);
  };
  add("Tillgångar", group("Summa tillgångar", [](std::string const& a) { return a.starts_with('1'); }));
  add("Eget kapital och skulder", group("Summa eget kapital och skulder",
                                        [](std::string const& a) { return a.starts_with('2'); }));
  auto other = group("Summa övriga balanskonton",
                     [](std::string const& a) { return !a.starts_with('1') && !a.starts_with('2'); });
  if (!other.first.empty()) add("Övriga balanskonton", other);

  // The year's result as the result accounts have it, not yet on a balance account
  Amount result = 0;
  for (auto const& account : accounts) {
    if (account_kind(books, account) == AccountKind::result) result += value_or_zero(b.change, account);
  }
  lines.push_back("");
  lines.push_back(balance_row("", format_amount(0), format_amount(result), format_amount(result), "Beräknat resultat"));
  lines.push_back(balance_row("", format_amount(total.opening), format_amount(total.change + result),
                              format_amount(total.closing + result), "Summa"));
  lines.push_back("");
  lines.push_back("Belopp: debet positivt, kredit negativt. Beräknat resultat är resultatkontonas summa (en vinst är negativ).");
  lines.push_back("Summa är noll när bokföringen går ihop.");
  if (total.opening != 0) {
    lines.push_back(std::format("De ingående balanserna går inte ihop: {}. Oftast är föregående års resultat inte "
                                "bokfört på eget kapital än.",
                                format_amount(total.opening)));
  }
  if (b.carried) {
    lines.push_back(std::format("Ingående balanser är förda från de utgående balanserna {}.",
                                format_iso_date(books.years[year_index - 1].end)));
    for (auto const& account : accounts_in({&b.opening, &year.reported_opening})) {
      auto const carried = value_or_zero(b.opening, account);
      auto const as_reported = value_or_zero(year.reported_opening, account);
      if (carried != as_reported) {
        lines.push_back(std::format("  {}: källan angav {}, förd {} (skillnad {})", account, format_amount(as_reported),
                                    format_amount(carried), format_amount(carried - as_reported)));
      }
    }
  } else {
    lines.push_back("Ingående balanser är som källan angav dem (året före finns inte i bokföringen).");
  }
  return lines;
}

std::vector<std::string> result_report(Books const& books, std::size_t year_index, std::string_view company) {
  auto const& year = books.years[year_index];
  auto const all = balances(books);
  auto const& b = all[year_index];
  auto lines = heading("Resultatrapport", year, company);
  lines.push_back(result_row("Konto", "Belopp", "Namn"));

  // Shown with income positive and costs negative: the books' amounts with the sign turned
  std::array<std::vector<std::string>, result_groups.size()> rows;
  std::array<Amount, result_groups.size()> sums{};
  std::vector<std::string> booking_rows;
  Amount booked = 0;
  for (auto const& account : accounts_in({&b.change})) {
    if (account_kind(books, account) != AccountKind::result) continue;
    auto const amount = -value_or_zero(b.change, account);
    if (amount == 0) continue;
    auto row = result_row(account, format_amount(amount), account_name(books, account));
    if (is_result_booking(account)) {
      booked += amount;
      booking_rows.push_back(std::move(row));
      continue;
    }
    auto const group = result_group_of(account);
    sums[group] += amount;
    rows[group].push_back(std::move(row));
  }
  Amount result = 0;
  for (std::size_t i = 0; i < result_groups.size(); ++i) {
    result += sums[i];
    if (rows[i].empty()) continue;
    lines.push_back("");
    lines.push_back(std::string(result_groups[i].heading));
    lines.insert(lines.end(), rows[i].begin(), rows[i].end());
    lines.push_back(result_row("", format_amount(sums[i]), result_groups[i].sum));
  }
  lines.push_back("");
  lines.push_back(result_row("", format_amount(result), "Årets resultat"));
  if (!booking_rows.empty()) {
    lines.push_back("");
    lines.push_back("Årets resultat bokfört (8990–8999)");
    lines.insert(lines.end(), booking_rows.begin(), booking_rows.end());
    lines.push_back(result_row("", format_amount(result + booked), "Kvar efter resultatbokningen"));
  }
  lines.push_back("");
  lines.push_back("Belopp: intäkter positiva, kostnader negativa. Ett positivt resultat är en vinst.");
  return lines;
}

std::vector<std::string> voucher_list(Books const& books, std::size_t year_index, std::string_view company) {
  auto const& year = books.years[year_index];
  auto lines = heading("Verifikationslista", year, company);
  lines.push_back(std::format("{} {}", year.vouchers.size(), year.vouchers.size() == 1 ? "verifikation" : "verifikationer"));
  for (auto const& voucher : year.vouchers) {
    lines.push_back("");
    lines.push_back(std::format(
        "{}  {}  {}{}", voucher_id(voucher.series, voucher.number), format_iso_date(voucher.event.date),
        voucher.event.heading,
        voucher.registered ? std::format("  (registrerad {})", format_iso_date(*voucher.registered)) : std::string{}));
    lines.push_back("  " + posting_row("Konto", {"Debet", "Kredit"}, ""));
    for (auto const& posting : voucher.postings) {
      auto text = posting.text.empty() ? account_name(books, posting.account) : posting.text;
      if (posting.kind != Posting::Kind::normal) {
        text += posting.kind == Posting::Kind::added ? "  (tillagd" : "  (struken";
        text += posting.signature.empty() ? ")" : std::format(" av {})", posting.signature);
      }
      lines.push_back("  " + posting_row(posting.account, sides_of(posting.amount), text));
    }
    lines.push_back("  " + posting_row("Summa", {format_amount(debit(voucher.postings)), format_amount(credit(voucher.postings))}, ""));
  }
  return lines;
}

std::vector<std::string> voucher_document(Books const& books, std::size_t year_index, Voucher const& voucher,
                                          std::string_view company) {
  auto const& year = books.years[year_index];
  auto const field = [](std::string_view label, std::string_view value) {
    return std::format("{}{}", left(std::string(label) + ":", 16), value);
  };
  std::vector<std::string> lines{std::format("Verifikation {}", voucher_id(voucher.series, voucher.number)),
                                 std::string(company), std::format("Räkenskapsår {}", year_text(year)), ""};
  auto const series_name = books.series_name(voucher.series);
  lines.push_back(field("Serie", series_name.empty() ? voucher.series : std::format("{}  {}", voucher.series, series_name)));
  lines.push_back(field("Datum", format_iso_date(voucher.event.date)));
  lines.push_back(field("Text", voucher.event.heading));
  lines.push_back(field("Belopp", format_amount(voucher.event.amount)));
  if (voucher.registered) lines.push_back(field("Registrerad", format_iso_date(*voucher.registered)));
  if (!voucher.signature.empty()) lines.push_back(field("Signatur", voucher.signature));
  lines.push_back("");
  lines.push_back(posting_row("Konto", {"Debet", "Kredit"}, ""));
  for (auto const& posting : voucher.postings) {
    auto text = account_name(books, posting.account);
    if (!posting.text.empty()) text += text.empty() ? posting.text : "  " + posting.text;
    if (posting.date && *posting.date != voucher.event.date) text += "  " + format_iso_date(*posting.date);
    if (posting.kind != Posting::Kind::normal) {
      text += posting.kind == Posting::Kind::added ? "  (tillagd" : "  (struken";
      text += posting.signature.empty() ? ")" : std::format(" av {})", posting.signature);
    }
    lines.push_back(posting_row(posting.account, sides_of(posting.amount), text));
  }
  lines.push_back(posting_row("Summa", {format_amount(debit(voucher.postings)), format_amount(credit(voucher.postings))}, ""));
  return lines;
}

namespace {

// The rows on account in the year (not struck ones), by date, ties in book order
struct Entry {
  Date date;
  Voucher const* voucher;
  Posting const* posting;
};
std::vector<Entry> entries_of(FiscalYear const& year, std::string_view account) {
  std::vector<Entry> entries;
  for (auto const& voucher : year.vouchers) {
    for (auto const& posting : voucher.postings) {
      if (posting.account == account && posting.kind != Posting::Kind::removed) {
        entries.push_back({posting.date.value_or(voucher.event.date), &voucher, &posting});
      }
    }
  }
  std::ranges::stable_sort(entries, {}, &Entry::date);
  return entries;
}

// One account's ledger without the report's heading: the columns, opening
// balance, the entries with the balance after each, and closing balance
std::vector<std::string> ledger_lines(Amount opening, std::vector<Entry> const& entries) {
  auto const row = [](std::string_view date, std::string_view voucher, std::string_view debit, std::string_view credit,
                      std::string_view balance, std::string_view text) {
    return with_rest(left(date, 12) + left(voucher, 14) + right(debit, 13) + right(credit, 13) + right(balance, 15), text);
  };
  // A balance line: the label over the date and voucher columns, the balance in its column
  auto const balance_line = [](std::string_view label, Amount amount) {
    return left(label, 26) + right("", 13) + right("", 13) + right(format_amount(amount), 15);
  };
  std::vector<std::string> lines{row("Datum", "Verifikation", "Debet", "Kredit", "Saldo", "Text")};
  Amount balance = opening;
  lines.push_back(balance_line("Ingående balans", balance));
  for (auto const& entry : entries) {
    balance += entry.posting->amount;
    auto const text = entry.posting->text.empty() ? entry.voucher->event.heading : entry.posting->text;
    auto const sides = sides_of(entry.posting->amount);
    lines.push_back(row(format_iso_date(entry.date), voucher_id(entry.voucher->series, entry.voucher->number),
                        sides.debit, sides.credit, format_amount(balance), text));
  }
  lines.push_back(balance_line("Utgående balans", balance));
  return lines;
}

std::string account_title(Books const& books, std::string const& number) {
  auto const name = account_name(books, number);
  return name.empty() ? std::format("Konto {}", number) : std::format("Konto {} {}", number, name);
}

} // namespace

std::optional<std::vector<std::string>> account_ledger(Books const& books, std::size_t year_index,
                                                       std::string_view company, std::string_view account) {
  auto const& year = books.years[year_index];
  auto const entries = entries_of(year, account);
  if (!books.account(account) && entries.empty()) return std::nullopt;
  auto const all = balances(books);
  auto const number = std::string(account);
  auto const name = account_name(books, account);
  auto lines = heading(std::format("Huvudbok konto {}{}", number, name.empty() ? "" : " " + name), year, company);
  auto const ledger = ledger_lines(value_or_zero(all[year_index].opening, number), entries);
  lines.insert(lines.end(), ledger.begin(), ledger.end());
  return lines;
}

std::vector<std::string> general_ledger(Books const& books, std::size_t year_index, std::string_view company) {
  auto const& year = books.years[year_index];
  auto const all = balances(books);
  auto const& b = all[year_index];
  auto lines = heading("Huvudbok", year, company);
  std::size_t shown = 0;
  for (auto const& account : accounts_in({&b.opening, &b.change})) {
    auto const entries = entries_of(year, account);
    auto const opening = value_or_zero(b.opening, account);
    if (entries.empty() && opening == 0) continue;
    if (shown++ > 0) lines.push_back("");
    lines.push_back(account_title(books, account));
    auto const ledger = ledger_lines(opening, entries);
    lines.insert(lines.end(), ledger.begin(), ledger.end());
  }
  if (shown == 0) lines.push_back("Inga konton har rader eller ingående balans under året.");
  return lines;
}

} // namespace cratchit::books
