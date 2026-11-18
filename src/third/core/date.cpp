#include "core/date.hpp"

#include <format>

namespace cratchit {

namespace {

std::optional<int> digits(std::string_view text) {
  if (text.empty()) {
    return std::nullopt;
  }
  int value = 0;
  for (char const c : text) {
    if (c < '0' || c > '9') {
      return std::nullopt;
    }
    value = value * 10 + (c - '0');
  }
  return value;
}

std::optional<Date> make_date(std::string_view year, std::string_view month, std::string_view day) {
  auto const y = digits(year);
  auto const m = digits(month);
  auto const d = digits(day);
  if (!y || !m || !d) {
    return std::nullopt;
  }
  Date const date{std::chrono::year{*y}, std::chrono::month{static_cast<unsigned>(*m)},
                  std::chrono::day{static_cast<unsigned>(*d)}};
  if (!date.ok()) {
    return std::nullopt;
  }
  return date;
}

} // namespace

std::optional<Date> parse_compact_date(std::string_view text) {
  if (text.size() != 8) {
    return std::nullopt;
  }
  return make_date(text.substr(0, 4), text.substr(4, 2), text.substr(6, 2));
}

std::optional<Date> parse_iso_date(std::string_view text) {
  if (text.size() != 10 || text[4] != '-' || text[7] != '-') {
    return std::nullopt;
  }
  return make_date(text.substr(0, 4), text.substr(5, 2), text.substr(8, 2));
}

std::string format_iso_date(Date date) {
  return std::format("{:04}-{:02}-{:02}", static_cast<int>(date.year()), static_cast<unsigned>(date.month()),
                     static_cast<unsigned>(date.day()));
}

std::string format_compact_date(Date date) {
  return std::format("{:04}{:02}{:02}", static_cast<int>(date.year()), static_cast<unsigned>(date.month()),
                     static_cast<unsigned>(date.day()));
}

Date next_day(Date date) {
  return Date{std::chrono::sys_days{date} + std::chrono::days{1}};
}

} // namespace cratchit
