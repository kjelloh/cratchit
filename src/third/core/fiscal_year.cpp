#include "core/fiscal_year.hpp"

#include "core/books/books.hpp"
#include "core/date.hpp"
#include "core/path_value.hpp"

#include <algorithm>
#include <chrono>
#include <format>
#include <system_error>

namespace cratchit {

namespace fs = std::filesystem;

namespace {

constexpr std::string_view year_prefix = "year.";

bool is_iso_date(std::string_view text) {
  if (text.size() != 10 || text[4] != '-' || text[7] != '-') return false;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (i == 4 || i == 7) continue;
    if (text[i] < '0' || text[i] > '9') return false;
  }
  return true;
}

} // namespace

std::expected<std::vector<FiscalYear>, std::string> list_fiscal_years(Project const& project) {
  auto const file = project.folder / ".cratchit" / "books" / "books.txt";
  std::error_code ec;
  if (!fs::exists(file, ec)) return std::vector<FiscalYear>{};
  auto const entries = read_path_values(file);
  if (!entries) return std::unexpected(entries.error());
  std::vector<FiscalYear> result;
  for (auto const& entry : *entries) {
    if (!entry.path.starts_with(year_prefix)) continue;
    FiscalYear year{entry.path.substr(year_prefix.size()), entry.value};
    if (!is_iso_date(year.start) || !is_iso_date(year.end)) {
      return std::unexpected(std::format("{}: '{} = {}' är inget räkenskapsår", file.string(), entry.path, entry.value));
    }
    result.push_back(std::move(year));
  }
  std::ranges::sort(result, std::ranges::greater{}, &FiscalYear::start);
  return result;
}

int years_before_current(FiscalYear const& year, std::string_view today) {
  auto const day = parse_iso_date(today);
  if (!day) return 0;
  int count = 0;
  for (auto current = year; *parse_iso_date(current.end) < *day; current = year_after(current)) ++count;
  return count;
}

FiscalYear year_after(FiscalYear const& year) {
  using namespace std::chrono;
  auto const end = *parse_iso_date(year.end);
  auto const start = next_day(end);
  auto const last_month = year_month(start.year(), start.month()) + months(11);
  return {format_iso_date(start), format_iso_date(year_month_day(last_month / last))};
}

FiscalYear year_before(FiscalYear const& year) {
  using namespace std::chrono;
  auto const start = *parse_iso_date(year.start);
  auto const end = sys_days(start) - days(1);
  auto const first_month = year_month(start.year(), start.month()) - months(12);
  return {format_iso_date(year_month_day(first_month / 1)), format_iso_date(year_month_day(end))};
}

int months_of(FiscalYear const& year) {
  auto const start = *parse_iso_date(year.start);
  auto const end = *parse_iso_date(year.end);
  return (static_cast<int>(end.year()) - static_cast<int>(start.year())) * 12 +
         static_cast<int>(static_cast<unsigned>(end.month())) - static_cast<int>(static_cast<unsigned>(start.month())) + 1;
}

std::expected<std::vector<ListedYear>, std::string> list_years_until(Project const& project, std::string_view today) {
  auto booked = list_fiscal_years(project); // the latest first
  if (!booked) return std::unexpected(booked.error());
  if (booked->empty()) return std::vector<ListedYear>{};
  auto const books = books::read_books(project.folder);
  if (!books) return std::unexpected(books.error());
  auto const vouchers_in = [&books](FiscalYear const& year) -> std::size_t {
    auto const start = parse_iso_date(year.start);
    auto const* found = start ? books->year_starting(*start) : nullptr;
    return found != nullptr ? found->vouchers.size() : 0;
  };
  // From the oldest in the books, each next one from the books when it is there, else twelve months on
  std::vector<FiscalYear> ascending(booked->rbegin(), booked->rend());
  std::vector<ListedYear> result;
  auto current = ascending.front();
  std::size_t next_booked = 1;
  for (;;) {
    bool const in_books = std::ranges::find(ascending, current) != ascending.end();
    result.push_back({current, in_books, vouchers_in(current)});
    bool const more_booked = next_booked < ascending.size();
    if (!more_booked && current.end >= today) break;
    auto next = year_after(current);
    if (more_booked && ascending[next_booked].start <= next.end) next = ascending[next_booked++];
    current = next;
  }
  // The year before the oldest in the books, to import an earlier year into
  result.insert(result.begin(), ListedYear{year_before(ascending.front()), false, 0});
  std::ranges::reverse(result);
  return result;
}

std::string label_of(FiscalYear const& year) {
  auto const start = year.start.substr(0, 4);
  auto const end = year.end.substr(0, 4);
  if (start == end) return start;
  if (start.substr(0, 2) == end.substr(0, 2)) return std::format("{}/{}", start, end.substr(2));
  return std::format("{}/{}", start, end);
}

} // namespace cratchit
