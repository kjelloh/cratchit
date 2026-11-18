#include "core/sie_summary.hpp"

#include "core/sie/document.hpp"
#include "core/sie/file.hpp"
#include "core/dot_folder.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <format>
#include <fstream>
#include <iterator>
#include <map>

namespace cratchit {

namespace fs = std::filesystem;

namespace {

// Voucher numbers compare as numbers when they are, else as text after them
bool number_less(std::string const& a, std::string const& b) {
  auto as_number = [](std::string const& text) -> std::optional<long long> {
    long long value = 0;
    auto const [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc() || end != text.data() + text.size()) return std::nullopt;
    return value;
  };
  auto const x = as_number(a);
  auto const y = as_number(b);
  if (x && y) return *x < *y;
  if (x != y) return x.has_value(); // numbers before text
  return a < b;
}

} // namespace

std::expected<SieSummary, std::string> summarize_sie_file(fs::path const& file) {
  std::ifstream in(file, std::ios::binary);
  if (!in) return std::unexpected(std::string("filen går inte att öppna"));
  std::vector<std::uint8_t> const bytes{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
  auto const lexed = sie::lex_bytes(bytes);
  if (!lexed) return std::unexpected(lexed.error());
  auto const parsed = sie::parse_document(lexed->lines);
  auto const& document = parsed.document;
  if (document.sie_type.empty()) return std::unexpected(std::string("#SIETYP saknas"));
  SieSummary summary;
  if (auto const year = document.year(0)) summary.year = std::pair{year->start, year->end};
  summary.generated = document.generated;
  summary.vouchers = document.vouchers.size();
  std::map<std::string, std::string> highest;
  auto const is_balance_account = [](std::string const& account) {
    return !account.empty() && (account.front() == '1' || account.front() == '2');
  };
  for (auto const& opening : document.opening) {
    if (opening.year == 0 && is_balance_account(opening.account)) summary.balance += opening.amount;
  }
  for (auto const& voucher : document.vouchers) {
    for (auto const& row : voucher.transactions) {
      if (row.kind == sie::Transaction::Kind::removed) continue; // struck, does not count
      if (is_balance_account(row.account)) summary.balance += row.amount;
      if (row.account == "8999") summary.result_booked = true;
    }
    if (!summary.last_date || *summary.last_date < voucher.date) summary.last_date = voucher.date;
    auto [it, added] = highest.try_emplace(voucher.series, voucher.number);
    if (!added && number_less(it->second, voucher.number)) it->second = voucher.number;
  }
  summary.series.assign(highest.begin(), highest.end());
  return summary;
}

std::optional<SieStatus> status_of(SieSummary const& summary, Date today) {
  if (!summary.year) return std::nullopt;
  auto const end = summary.year->second;
  if (!(end < today)) return SieStatus{YearStatus::ongoing, false};
  if (summary.balance == 0) return SieStatus{YearStatus::complete, false};
  if (summary.result_booked) return SieStatus{YearStatus::mismatch, false};
  // Last year may lack its closing still; a year before it must not
  auto year_after_end = std::chrono::year_month_day(end.year() + std::chrono::years(1), end.month(), end.day());
  if (!year_after_end.ok()) year_after_end = std::chrono::year_month_day(year_after_end.year() / year_after_end.month() / std::chrono::last);
  return SieStatus{YearStatus::incomplete, year_after_end < today};
}

std::string status_text(SieStatus status) {
  std::string text;
  switch (status.status) {
  case YearStatus::ongoing: text = "pågående"; break;
  case YearStatus::incomplete: text = "ofullbordat"; break;
  case YearStatus::complete: text = "fullbordat"; break;
  case YearStatus::mismatch: text = "misstämmer"; break;
  }
  return status.warning ? text + " !" : text;
}

Date today_date() { return parse_iso_date(today_iso()).value_or(Date{}); }

std::string summary_line(SieSummary const& summary, Date today) {
  std::vector<std::string> parts;
  if (summary.year) parts.push_back(std::format("{} – {}", format_iso_date(summary.year->first), format_iso_date(summary.year->second)));
  if (auto const status = status_of(summary, today)) parts.push_back(status_text(*status));
  if (summary.generated) parts.push_back(std::format("skapad {}", format_iso_date(*summary.generated)));
  if (summary.last_date) {
    parts.push_back(std::format("{} ver. t.o.m. {}", summary.vouchers, format_iso_date(*summary.last_date)));
  } else {
    parts.push_back(std::format("{} ver.", summary.vouchers));
  }
  if (!summary.series.empty()) {
    std::string series;
    for (auto const& [name, number] : summary.series) {
      if (!series.empty()) series += " · ";
      series += std::format("{}{}", name, number);
    }
    parts.push_back(std::move(series));
  }
  std::string line;
  for (auto const& part : parts) {
    if (!line.empty()) line += "  ";
    line += part;
  }
  return line;
}

namespace {

// The width of UTF-8 text in the terminal, counted in code points
std::size_t width_of(std::string_view text) {
  return static_cast<std::size_t>(std::ranges::count_if(text, [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; }));
}

std::string padded(std::string const& text, std::size_t width) {
  auto const used = width_of(text);
  return used < width ? text + std::string(width - used, ' ') : text;
}

std::string series_text(SieSummary const& summary) {
  std::string series;
  for (auto const& [name, number] : summary.series) {
    if (!series.empty()) series += " · ";
    series += std::format("{}{}", name, number);
  }
  return series;
}

std::string year_text(std::pair<Date, Date> const& year) {
  return std::format("{} – {}", format_iso_date(year.first), format_iso_date(year.second));
}

struct Listed {
  std::string name;
  std::expected<SieSummary, std::string> summary;
};

} // namespace

std::vector<std::string> list_sie_files(std::vector<fs::path> const& files, std::vector<fs::path> const& folders) {
  std::vector<Listed> listed;
  for (auto const& file : files) {
    auto const name = file.filename().u8string();
    listed.push_back({std::string(name.begin(), name.end()), summarize_sie_file(file)});
  }
  // Columns as wide as their widest entry, the same in every group
  auto const today = today_date();
  auto const status_column = [today](SieSummary const& summary) {
    auto const status = status_of(summary, today);
    return status ? status_text(*status) : std::string();
  };
  std::size_t name_width = 0;
  std::size_t count_width = 0;
  std::size_t status_width = 0;
  for (auto const& entry : listed) {
    name_width = std::max(name_width, width_of(entry.name));
    if (!entry.summary) continue;
    count_width = std::max(count_width, std::to_string(entry.summary->vouchers).size());
    status_width = std::max(status_width, width_of(status_column(*entry.summary)));
  }
  auto const line_of = [&](Listed const& entry) {
    auto const& summary = *entry.summary;
    auto line = "  " + padded(entry.name, name_width);
    if (status_width > 0) line += "  " + padded(status_column(summary), status_width);
    line += "  " + padded(summary.generated ? "skapad " + format_iso_date(*summary.generated) : std::string(), 17);
    auto const count = std::to_string(summary.vouchers);
    line += "  " + std::string(count_width - count.size(), ' ') + count + " ver.";
    line += "  " + padded(summary.last_date ? "t.o.m. " + format_iso_date(*summary.last_date) : std::string(), 17);
    if (auto const series = series_text(summary); !series.empty()) line += "  " + series;
    while (!line.empty() && line.back() == ' ') line.pop_back();
    return line;
  };

  // The fiscal years, latest first; within one, the latest created first
  std::vector<Listed const*> with_year;
  std::vector<Listed const*> without_year;
  std::vector<Listed const*> unreadable;
  for (auto const& entry : listed) {
    if (!entry.summary) unreadable.push_back(&entry);
    else if (entry.summary->year) with_year.push_back(&entry);
    else without_year.push_back(&entry);
  }
  std::ranges::stable_sort(with_year, [](Listed const* a, Listed const* b) {
    auto const& x = *a->summary;
    auto const& y = *b->summary;
    if (x.year != y.year) return *y.year < *x.year;
    return y.generated < x.generated; // an unknown created date (nullopt) sorts last
  });
  std::ranges::stable_sort(without_year, [](Listed const* a, Listed const* b) {
    return b->summary->generated < a->summary->generated;
  });

  std::vector<std::string> lines;
  std::optional<std::pair<Date, Date>> heading;
  for (auto const* entry : with_year) {
    if (heading != entry->summary->year) {
      heading = entry->summary->year;
      lines.push_back(year_text(*heading));
    }
    lines.push_back(line_of(*entry));
  }
  if (!without_year.empty()) {
    lines.push_back("Utan räkenskapsår");
    for (auto const* entry : without_year) lines.push_back(line_of(*entry));
  }
  if (!unreadable.empty()) {
    lines.push_back("Går inte att läsa");
    for (auto const* entry : unreadable) lines.push_back("  " + padded(entry->name, name_width) + "  " + entry->summary.error());
  }
  if (!folders.empty()) {
    lines.push_back("Mappar");
    for (auto const& folder : folders) {
      auto const name = folder.filename().u8string();
      lines.push_back("  " + std::string(name.begin(), name.end()) + static_cast<char>(fs::path::preferred_separator));
    }
  }
  return lines;
}

std::vector<std::string> describe_sie_file(fs::path const& file) {
  auto const summary = summarize_sie_file(file);
  if (!summary) return {std::format("(går inte att läsa: {})", summary.error())};
  return {summary_line(*summary, today_date())};
}

} // namespace cratchit
