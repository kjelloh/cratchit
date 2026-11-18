#include "core/books/books.hpp"

#include "core/csv.hpp"
#include "core/path_value.hpp"

#include <algorithm>
#include <chrono>
#include <ranges>
#include <format>
#include <fstream>
#include <sstream>

namespace cratchit::books {

std::string voucher_id(std::string_view series, std::string_view number) {
  return std::string(series) + std::string(number);
}

std::optional<std::pair<std::string, std::string>> parse_voucher_id(std::string_view text) {
  auto const digits = text.find_first_of("0123456789");
  if (digits == 0 || digits == std::string_view::npos) return std::nullopt;
  auto const series = text.substr(0, digits);
  auto const number = text.substr(digits);
  if (series.find_first_of(" \t0123456789") != std::string_view::npos) return std::nullopt;
  if (number.find_first_not_of("0123456789") != std::string_view::npos) return std::nullopt;
  return std::pair{std::string(series), std::string(number)};
}


namespace fs = std::filesystem;

namespace {

constexpr std::string_view books_file = "books.txt";
constexpr std::string_view accounts_file = "accounts.csv";
constexpr std::string_view dimensions_file = "dimensions.csv";
constexpr std::string_view objects_file = "objects.csv";
constexpr std::string_view opening_file = "opening.csv";
constexpr std::string_view vouchers_file = "vouchers.csv";
constexpr std::string_view postings_file = "postings.csv";
constexpr std::string_view old_postings_file = "transactions.csv"; // the name of postings.csv before 2026-10-24
constexpr std::string_view events_file = "events.csv";
constexpr std::string_view series_file = "series.csv";
constexpr std::string_view event_postings_file = "event_postings.csv";

CsvRow const accounts_header{"account", "name", "type", "sru", "unit"};
CsvRow const series_header{"series", "name"};
CsvRow const dimensions_header{"dimension", "name", "super"};
CsvRow const objects_header{"dimension", "object", "name"};
CsvRow const opening_header{"account", "amount"};
CsvRow const vouchers_header{"series", "number", "date", "text", "amount", "registered", "signature"};
CsvRow const events_header{"date", "text", "amount", "noted", "series"};
// events.csv as written before drafts had a series (2026-11-18): read as with no series
CsvRow const events_header_without_series{"date", "text", "amount", "noted"};
// event is the row of the event in events.csv, 1 for the first
CsvRow const event_postings_header{"event", "account", "amount", "date", "text", "source"};
CsvRow const postings_header{"series", "number", "kind",     "account",  "objects",
                             "amount", "date",   "text",     "quantity", "signature"};

// Each series the vouchers use is in the list (unnamed when new), and the list is by series
void list_used_series(Books& books) {
  for (auto const& year : books.years) {
    for (auto const& voucher : year.vouchers) {
      if (std::ranges::find(books.series, voucher.series, &Series::series) == books.series.end()) {
        books.series.push_back({voucher.series, {}});
      }
    }
  }
  std::ranges::sort(books.series, {}, &Series::series);
}

bool all_digits(std::string_view text) {
  return !text.empty() && std::ranges::all_of(text, [](char c) { return c >= '0' && c <= '9'; });
}

std::string_view kind_name(Posting::Kind kind) {
  switch (kind) {
  case Posting::Kind::added:
    return "added";
  case Posting::Kind::removed:
    return "removed";
  default:
    return "normal";
  }
}

std::optional<Posting::Kind> kind_from_name(std::string_view name) {
  if (name == "normal") {
    return Posting::Kind::normal;
  }
  if (name == "added") {
    return Posting::Kind::added;
  }
  if (name == "removed") {
    return Posting::Kind::removed;
  }
  return std::nullopt;
}

// Object pairs as one CSV field: the items as a CSV line of their own
std::string format_objects(Objects const& objects) {
  if (objects.empty()) {
    return {};
  }
  CsvRow items;
  for (auto const& [dimension, object] : objects) {
    items.push_back(dimension);
    items.push_back(object);
  }
  auto line = format_csv_row(items);
  line.pop_back();
  return line;
}

std::optional<Objects> parse_objects(std::string_view text) {
  if (text.empty()) {
    return Objects{};
  }
  auto const rows = parse_csv(text);
  if (!rows || rows->size() != 1 || rows->front().size() % 2 != 0) {
    return std::nullopt;
  }
  Objects objects;
  auto const& items = rows->front();
  for (std::size_t i = 0; i < items.size(); i += 2) {
    objects.emplace_back(items[i], items[i + 1]);
  }
  return objects;
}

std::string optional_date_text(std::optional<Date> const& date) {
  return date ? format_iso_date(*date) : std::string{};
}

std::string csv_text(CsvRow const& header, std::vector<CsvRow> const& rows) {
  std::string text = format_csv_row(header);
  for (auto const& row : rows) {
    text += format_csv_row(row);
  }
  return text;
}

std::string year_folder_name(Date start) {
  return format_iso_date(start);
}

// The rows of a CSV file after its header, each with the header's number of fields
std::expected<std::vector<CsvRow>, std::string> read_table(fs::path const& file, CsvRow const& header) {
  auto rows = read_csv(file);
  if (!rows) {
    return std::unexpected(rows.error());
  }
  if (rows->empty() || rows->front() != header) {
    return std::unexpected(std::format("{}: the first line must be {}", file.string(),
                                       format_csv_row(header).substr(0, format_csv_row(header).size() - 1)));
  }
  rows->erase(rows->begin());
  for (std::size_t i = 0; i < rows->size(); ++i) {
    if ((*rows)[i].size() != header.size()) {
      return std::unexpected(
          std::format("{}: row {} has {} fields, not {}", file.string(), i + 2, (*rows)[i].size(), header.size()));
    }
  }
  return rows;
}

std::expected<Date, std::string> date_field(fs::path const& file, std::string const& text) {
  auto const date = parse_iso_date(text);
  if (!date) {
    return std::unexpected(std::format("{}: '{}' is not a date YYYY-MM-DD", file.string(), text));
  }
  return *date;
}

std::expected<std::optional<Date>, std::string> optional_date_field(fs::path const& file, std::string const& text) {
  if (text.empty()) {
    return std::optional<Date>{};
  }
  auto const date = date_field(file, text);
  if (!date) {
    return std::unexpected(date.error());
  }
  return std::optional<Date>{*date};
}

// As read_table, but a missing file is an empty table: books made from
// details have no chart of accounts, dimensions or objects yet
std::expected<std::vector<CsvRow>, std::string> read_optional_table(fs::path const& file, CsvRow const& header) {
  std::error_code ec;
  if (!fs::exists(file, ec)) {
    return std::vector<CsvRow>{};
  }
  return read_table(file, header);
}

std::expected<Amount, std::string> amount_field(fs::path const& file, std::string const& text) {
  auto const amount = parse_amount(text);
  if (!amount) {
    return std::unexpected(std::format("{}: '{}' is not an amount", file.string(), text));
  }
  return *amount;
}

std::expected<FiscalYear, std::string> read_year(fs::path const& folder, Date start, Date end) {
  FiscalYear year{start, end, {}, {}};
  auto const year_folder = folder / year_folder_name(start);
  std::error_code missing;
  if (!fs::exists(year_folder, missing)) {
    return year; // a year without a folder has no balances or vouchers yet, e.g. in books made from details
  }
  auto const opening_path = year_folder / opening_file;
  auto const opening = read_table(opening_path, opening_header);
  if (!opening) {
    return std::unexpected(opening.error());
  }
  for (auto const& row : *opening) {
    auto const amount = amount_field(opening_path, row[1]);
    if (!amount) {
      return std::unexpected(amount.error());
    }
    year.reported_opening[row[0]] = *amount;
  }
  auto const vouchers_path = year_folder / vouchers_file;
  auto const vouchers = read_table(vouchers_path, vouchers_header);
  if (!vouchers) {
    return std::unexpected(vouchers.error());
  }
  for (auto const& row : *vouchers) {
    auto const date = date_field(vouchers_path, row[2]);
    if (!date) {
      return std::unexpected(date.error());
    }
    auto const amount = amount_field(vouchers_path, row[4]);
    if (!amount) {
      return std::unexpected(amount.error());
    }
    auto const registered = optional_date_field(vouchers_path, row[5]);
    if (!registered) {
      return std::unexpected(registered.error());
    }
    year.vouchers.push_back({row[0], row[1], {row[3], *amount, *date}, *registered, row[6], {}});
  }
  std::error_code ec;
  auto postings_path = year_folder / postings_file;
  if (!fs::exists(postings_path, ec) && fs::exists(year_folder / old_postings_file, ec)) {
    postings_path = year_folder / old_postings_file;
  }
  auto const postings = read_table(postings_path, postings_header);
  if (!postings) {
    return std::unexpected(postings.error());
  }
  for (auto const& row : *postings) {
    auto const voucher = std::ranges::find_if(
        year.vouchers, [&](Voucher const& v) { return v.series == row[0] && v.number == row[1]; });
    if (voucher == year.vouchers.end()) {
      return std::unexpected(
          std::format("{}: voucher {} {} is not in {}", postings_path.string(), row[0], row[1], vouchers_file));
    }
    auto const kind = kind_from_name(row[2]);
    auto const objects = parse_objects(row[4]);
    auto const amount = amount_field(postings_path, row[5]);
    auto const date = optional_date_field(postings_path, row[6]);
    if (!kind || !objects) {
      return std::unexpected(std::format("{}: '{}' is not a kind or '{}' not objects", postings_path.string(),
                                         row[2], row[4]));
    }
    if (!amount) {
      return std::unexpected(amount.error());
    }
    if (!date) {
      return std::unexpected(date.error());
    }
    voucher->postings.push_back({*kind, row[3], *objects, *amount, *date, row[7], row[8], row[9]});
  }
  auto const events_path = year_folder / events_file;
  if (!fs::exists(events_path, ec)) {
    return year;
  }
  auto events = read_table(events_path, events_header);
  if (!events) {
    auto older = read_table(events_path, events_header_without_series);
    if (!older) return std::unexpected(events.error());
    for (auto& row : *older) row.emplace_back();
    events = std::move(older);
  }
  for (auto const& row : *events) {
    auto const date = date_field(events_path, row[0]);
    if (!date) {
      return std::unexpected(date.error());
    }
    auto const amount = amount_field(events_path, row[2]);
    if (!amount) {
      return std::unexpected(amount.error());
    }
    auto const noted = date_field(events_path, row[3]);
    if (!noted) {
      return std::unexpected(noted.error());
    }
    year.drafts.push_back({{row[1], *amount, *date}, *noted, {}, row[4]});
  }
  auto const event_postings_path = year_folder / event_postings_file;
  if (fs::exists(event_postings_path, ec)) {
    auto const event_postings = read_table(event_postings_path, event_postings_header);
    if (!event_postings) {
      return std::unexpected(event_postings.error());
    }
    for (auto const& row : *event_postings) {
      auto const event = all_digits(row[0]) && row[0].size() < 9 ? std::stoul(row[0]) : 0;
      if (event == 0 || event > year.drafts.size()) {
        return std::unexpected(
            std::format("{}: event {} is not in {}", event_postings_path.string(), row[0], events_file));
      }
      auto const amount = amount_field(event_postings_path, row[2]);
      if (!amount) {
        return std::unexpected(amount.error());
      }
      auto const date = optional_date_field(event_postings_path, row[3]);
      if (!date) {
        return std::unexpected(date.error());
      }
      Posting posting;
      posting.account = row[1];
      posting.amount = *amount;
      posting.date = *date;
      posting.text = row[4];
      posting.source = row[5];
      year.drafts[event - 1].postings.push_back(std::move(posting));
    }
  }
  // Files written before the date order are in the order noted
  std::ranges::stable_sort(year.drafts, {}, [](Draft const& draft) { return draft.event.date; });
  return year;
}

// Writes text to file when it differs from what the file holds; whether it was written and created
std::expected<std::optional<bool>, std::string> write_if_changed(fs::path const& file, std::string const& text) {
  std::error_code ec;
  bool const exists = fs::exists(file, ec);
  if (exists) {
    std::ifstream in(file, std::ios::binary);
    std::ostringstream current;
    current << in.rdbuf();
    if (current.str() == text) {
      return std::optional<bool>{};
    }
  }
  fs::create_directories(file.parent_path(), ec);
  std::ofstream out(file, std::ios::binary | std::ios::trunc);
  out << text;
  if (!out) {
    return std::unexpected(std::format("Cannot write {}", file.string()));
  }
  return std::optional<bool>{!exists};
}

bool number_less(std::string const& a, std::string const& b) {
  return a.size() != b.size() ? a.size() < b.size() : a < b;
}

} // namespace

Amount debit(Postings const& postings) {
  Amount total = 0;
  for (auto const& posting : postings) {
    if (posting.kind != Posting::Kind::removed && posting.amount > 0) {
      total += posting.amount;
    }
  }
  return total;
}

Amount credit(Postings const& postings) {
  Amount total = 0;
  for (auto const& posting : postings) {
    if (posting.kind != Posting::Kind::removed && posting.amount < 0) {
      total -= posting.amount;
    }
  }
  return total;
}

Amount sum(Postings const& postings) {
  return debit(postings) - credit(postings);
}

DraftParts parts_of(Draft const& draft) {
  std::vector<Amount> amounts;
  auto const without_sign = [](Amount amount) { return amount < 0 ? -amount : amount; };
  auto const has_event = !draft.event.heading.empty() || draft.event.amount != 0;
  if (has_event) amounts.push_back(without_sign(draft.event.amount));
  for (auto const& posting : draft.postings) {
    if (posting.kind != Posting::Kind::removed) amounts.push_back(without_sign(posting.amount));
  }
  if (amounts.size() <= 1) return DraftParts::lone;
  // An event without amount (a heading alone) says nothing about the amount
  auto const first = has_event && draft.event.amount == 0 ? 1u : 0u;
  auto const same = std::ranges::all_of(amounts | std::views::drop(first),
                                        [&](Amount amount) { return amount == amounts[first]; });
  return same ? DraftParts::same : DraftParts::more;
}

DraftStage stage(Draft const& draft) {
  auto const counted = [](Posting const& posting) { return posting.kind != Posting::Kind::removed; };
  if (std::ranges::none_of(draft.postings, counted)) {
    return DraftStage::event;
  }
  if (draft.event.heading.empty() && draft.event.amount == 0 && std::ranges::count_if(draft.postings, counted) == 1) {
    return DraftStage::account_event;
  }
  auto const on_accounts = std::ranges::all_of(draft.postings, [&](Posting const& posting) {
    return !counted(posting) || !posting.account.empty();
  });
  // The amount has its sign in a draft (minus for an event the other way); debit adds up to it without sign
  auto const amount = draft.event.amount < 0 ? -draft.event.amount : draft.event.amount;
  auto const ready = on_accounts && !draft.event.heading.empty() && amount != 0 && sum(draft.postings) == 0 && debit(draft.postings) == amount;
  if (ready) return DraftStage::ready;
  return parts_of(draft) == DraftParts::same ? DraftStage::same : DraftStage::unbalanced;
}

FiscalYear* continued_year(Books& books, Date start) {
  using namespace std::chrono;
  auto const found = std::ranges::find(books.years, start, &FiscalYear::start);
  if (found != books.years.end()) return &*found;
  if (books.years.empty() || start <= books.years.back().end) return nullptr;
  // Year after year from the last in the books, until start is reached or passed
  std::vector<FiscalYear> added;
  auto end = books.years.back().end;
  while (true) {
    auto const next = Date{sys_days(end) + days(1)};
    if (next > start) return nullptr;
    auto const last_month = year_month(next.year(), next.month()) + months(11);
    end = Date{last_month / last};
    added.push_back(FiscalYear{next, end});
    if (next == start) break;
  }
  for (auto& year : added) books.years.push_back(std::move(year));
  return &books.years.back();
}

bool FiscalYear::holds(Date date) const {
  return start <= date && date <= end;
}

std::size_t FiscalYear::place_draft(Draft draft) {
  auto const at = std::ranges::upper_bound(drafts, draft.event.date, {},
                                           [](Draft const& other) { return other.event.date; });
  // The index first: insert may move the elements, and C++ does not say which side of a - b is evaluated
  // first, so insert(...) - begin() may take begin() from before the move (GCC 14 does)
  auto const index = static_cast<std::size_t>(at - drafts.begin());
  drafts.insert(at, std::move(draft));
  return index;
}

std::vector<Series> const& default_series() {
  static std::vector<Series> const series{
      {"A", "Redovisning"},       {"B", "Kundfakturor"},  {"C", "Inbetalningar från kunder"},
      {"D", "Leverantörsfakturor"}, {"E", "Utbetalningar till leverantörer"}, {"F", "Kassa"},
      {"G", "Avskrivning"},       {"H", "Periodisering"}, {"I", "Bokslut"},
      {"J", "Kontantfaktura"},    {"K", "Kvitto"},        {"L", "Lön"},
      {"M", "Momsrapport"},       {"N", "Lager"},
  };
  return series;
}

std::string Books::series_name(std::string_view name) const {
  auto const found = std::ranges::find(series, name, &Series::series);
  return found != series.end() ? found->name : std::string();
}

FiscalYear const* Books::year_starting(Date start) const {
  auto const it = std::ranges::find(years, start, &FiscalYear::start);
  return it == years.end() ? nullptr : &*it;
}

Account const* Books::account(std::string_view number) const {
  auto const it = std::ranges::find(accounts, number, &Account::number);
  return it == accounts.end() ? nullptr : &*it;
}

AccountKind account_kind(Books const& books, std::string_view number) {
  auto const* account = books.account(number);
  return cratchit::account_kind(number, account ? std::string_view{account->type} : std::string_view{});
}

std::vector<YearBalances> balances(Books const& books) {
  std::vector<YearBalances> result;
  for (std::size_t i = 0; i < books.years.size(); ++i) {
    auto const& year = books.years[i];
    YearBalances balances;
    balances.carried = i > 0 && next_day(books.years[i - 1].end) == year.start;
    if (balances.carried) {
      for (auto const& [account, amount] : result.back().closing) {
        if (account_kind(books, account) == AccountKind::balance) {
          balances.opening[account] = amount;
        }
      }
    } else {
      balances.opening = year.reported_opening;
    }
    for (auto const& voucher : year.vouchers) {
      for (auto const& posting : voucher.postings) {
        if (posting.kind != Posting::Kind::removed) {
          balances.change[posting.account] += posting.amount;
        }
      }
    }
    balances.closing = balances.opening;
    for (auto const& [account, amount] : balances.change) {
      balances.closing[account] += amount;
    }
    result.push_back(std::move(balances));
  }
  return result;
}

fs::path books_folder(fs::path const& project_folder) {
  return project_folder / ".cratchit" / "books";
}

std::expected<Books, std::string> read_books(fs::path const& project_folder) {
  auto const folder = books_folder(project_folder);
  std::error_code ec;
  if (!fs::exists(folder / books_file, ec)) {
    return Books{};
  }
  auto const entries = read_path_values(folder / books_file);
  if (!entries) {
    return std::unexpected(entries.error());
  }
  Books books;
  std::vector<std::pair<Date, Date>> years;
  std::vector<Date> closed;
  for (auto const& [path, value] : *entries) {
    if (path == "chart_type") {
      books.chart_type = value;
    } else if (path == "company_id") {
      books.company_id = value;
    } else if (path == "address.contact") {
      books.address.contact = value;
    } else if (path == "address.street") {
      books.address.street = value;
    } else if (path == "address.postal") {
      books.address.postal = value;
    } else if (path == "address.phone") {
      books.address.phone = value;
    } else if (path.starts_with("year.")) {
      auto const start = parse_iso_date(std::string_view{path}.substr(5));
      auto const end = parse_iso_date(value);
      if (!start || !end) {
        return std::unexpected(std::format("{}: '{} = {}' is not 'year.<YYYY-MM-DD> = <YYYY-MM-DD>'",
                                           (folder / books_file).string(), path, value));
      }
      years.emplace_back(*start, *end);
    } else if (path.starts_with("closed.")) {
      auto const start = parse_iso_date(std::string_view{path}.substr(7));
      if (!start || value != "yes") {
        return std::unexpected(std::format("{}: '{} = {}' is not 'closed.<YYYY-MM-DD> = yes'",
                                           (folder / books_file).string(), path, value));
      }
      closed.push_back(*start);
    }
  }
  auto const accounts = read_optional_table(folder / accounts_file, accounts_header);
  if (!accounts) {
    return std::unexpected(accounts.error());
  }
  for (auto const& row : *accounts) {
    books.accounts.push_back({row[0], row[1], row[2], row[3], row[4]});
  }
  auto const dimensions = read_optional_table(folder / dimensions_file, dimensions_header);
  if (!dimensions) {
    return std::unexpected(dimensions.error());
  }
  for (auto const& row : *dimensions) {
    books.dimensions.push_back({row[0], row[1], row[2]});
  }
  auto const objects = read_optional_table(folder / objects_file, objects_header);
  if (!objects) {
    return std::unexpected(objects.error());
  }
  for (auto const& row : *objects) {
    books.objects.push_back({row[0], row[1], row[2]});
  }
  std::error_code series_ec;
  if (fs::exists(folder / series_file, series_ec)) {
    auto const series = read_optional_table(folder / series_file, series_header);
    if (!series) return std::unexpected(series.error());
    for (auto const& row : *series) books.series.push_back({row[0], row[1]});
  } else {
    books.series = default_series();
  }
  std::ranges::sort(years);
  for (auto const& [start, end] : years) {
    auto year = read_year(folder, start, end);
    if (!year) {
      return std::unexpected(year.error());
    }
    year->closed = std::ranges::contains(closed, start);
    books.years.push_back(std::move(*year));
  }
  list_used_series(books);
  return books;
}

std::expected<std::vector<Written>, std::string> write_books(fs::path const& project_folder, Books const& books) {
  auto const folder = books_folder(project_folder);
  std::vector<std::pair<fs::path, std::string>> files;
  std::vector<fs::path> gone; // files that hold nothing, removed if they exist

  PathValues entries{{"chart_type", books.chart_type},
                     {"company_id", books.company_id},
                     {"address.contact", books.address.contact},
                     {"address.street", books.address.street},
                     {"address.postal", books.address.postal},
                     {"address.phone", books.address.phone}};
  auto years = books.years;
  std::ranges::sort(years, {}, &FiscalYear::start);
  for (auto const& year : years) {
    entries.push_back({"year." + format_iso_date(year.start), format_iso_date(year.end)});
  }
  for (auto const& year : years) {
    if (year.closed) {
      entries.push_back({"closed." + format_iso_date(year.start), "yes"});
    }
  }
  files.emplace_back(books_file, format_path_values(entries));

  auto accounts = books.accounts;
  std::ranges::sort(accounts, number_less, &Account::number);
  std::vector<CsvRow> rows;
  for (auto const& account : accounts) {
    rows.push_back({account.number, account.name, account.type, account.sru, account.unit});
  }
  files.emplace_back(accounts_file, csv_text(accounts_header, rows));
  rows.clear();
  for (auto const& dimension : books.dimensions) {
    rows.push_back({dimension.number, dimension.name, dimension.super});
  }
  files.emplace_back(dimensions_file, csv_text(dimensions_header, rows));
  rows.clear();
  for (auto const& object : books.objects) {
    rows.push_back({object.dimension, object.number, object.name});
  }
  files.emplace_back(objects_file, csv_text(objects_header, rows));
  rows.clear();
  auto listed = books;
  list_used_series(listed);
  for (auto const& series : listed.series) rows.push_back({series.series, series.name});
  files.emplace_back(series_file, csv_text(series_header, rows));

  for (auto const& year : years) {
    auto const year_folder = fs::path(year_folder_name(year.start));
    rows.clear();
    for (auto const& [account, amount] : year.reported_opening) {
      rows.push_back({account, format_amount(amount)});
    }
    files.emplace_back(year_folder / opening_file, csv_text(opening_header, rows));
    rows.clear();
    std::vector<CsvRow> posting_rows;
    for (auto const& voucher : year.vouchers) {
      rows.push_back({voucher.series, voucher.number, format_iso_date(voucher.event.date), voucher.event.heading,
                      format_amount(voucher.event.amount), optional_date_text(voucher.registered), voucher.signature});
      for (auto const& t : voucher.postings) {
        posting_rows.push_back({voucher.series, voucher.number, std::string(kind_name(t.kind)), t.account,
                                    format_objects(t.objects), format_amount(t.amount), optional_date_text(t.date),
                                    t.text, t.quantity, t.signature});
      }
    }
    files.emplace_back(year_folder / vouchers_file, csv_text(vouchers_header, rows));
    files.emplace_back(year_folder / postings_file, csv_text(postings_header, posting_rows));
    gone.push_back(year_folder / old_postings_file);
    rows.clear();
    for (auto const& draft : year.drafts) {
      rows.push_back({format_iso_date(draft.event.date), draft.event.heading, format_amount(draft.event.amount),
                      format_iso_date(draft.noted), draft.series});
    }
    if (rows.empty()) {
      gone.push_back(year_folder / events_file);
    } else {
      files.emplace_back(year_folder / events_file, csv_text(events_header, rows));
    }
    rows.clear();
    for (std::size_t i = 0; i < year.drafts.size(); ++i) {
      for (auto const& posting : year.drafts[i].postings) {
        rows.push_back({std::to_string(i + 1), posting.account, format_amount(posting.amount),
                        optional_date_text(posting.date), posting.text, posting.source});
      }
    }
    if (rows.empty()) {
      gone.push_back(year_folder / event_postings_file);
    } else {
      files.emplace_back(year_folder / event_postings_file, csv_text(event_postings_header, rows));
    }
  }

  std::vector<Written> written;
  for (auto const& [file, text] : files) {
    auto const result = write_if_changed(folder / file, text);
    if (!result) {
      return std::unexpected(result.error());
    }
    if (*result) {
      written.push_back({fs::path(".cratchit") / "books" / file, **result});
    }
  }
  // Removed after the writes, so a failed write keeps what the file held
  for (auto const& file : gone) {
    std::error_code ec;
    if (fs::exists(folder / file, ec)) {
      if (!fs::remove(folder / file, ec)) {
        return std::unexpected(std::format("Cannot remove {}", (folder / file).string()));
      }
      written.push_back({fs::path(".cratchit") / "books" / file, false, true});
    }
  }
  return written;
}

} // namespace cratchit::books
