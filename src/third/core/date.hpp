#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

namespace cratchit {

using Date = std::chrono::year_month_day;

// A valid date written YYYYMMDD, as in SIE
std::optional<Date> parse_compact_date(std::string_view text);

// A valid date written YYYY-MM-DD
std::optional<Date> parse_iso_date(std::string_view text);

// YYYY-MM-DD
std::string format_iso_date(Date date);

// YYYYMMDD, as in SIE and SRU
std::string format_compact_date(Date date);

// The day after date
Date next_day(Date date);

} // namespace cratchit
