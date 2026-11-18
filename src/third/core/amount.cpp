#include "core/amount.hpp"

#include <cstdint>
#include <format>

namespace cratchit {

std::optional<Amount> parse_amount(std::string_view text) {
  bool const negative = text.starts_with('-');
  if (negative) {
    text.remove_prefix(1);
  }
  auto const point = text.find('.');
  auto const whole = text.substr(0, point);
  auto const decimals = point == std::string_view::npos ? std::string_view{} : text.substr(point + 1);
  if (whole.empty() || whole.size() > 15 || (point != std::string_view::npos && decimals.empty()) ||
      decimals.size() > 2) {
    return std::nullopt;
  }
  Amount ore = 0;
  for (char const c : whole) {
    if (c < '0' || c > '9') {
      return std::nullopt;
    }
    ore = ore * 10 + (c - '0');
  }
  Amount fraction = 0;
  for (char const c : decimals) {
    if (c < '0' || c > '9') {
      return std::nullopt;
    }
    fraction = fraction * 10 + (c - '0');
  }
  if (decimals.size() == 1) {
    fraction *= 10;
  }
  ore = ore * 100 + fraction;
  return negative ? -ore : ore;
}

std::string format_amount(Amount amount) {
  auto const magnitude = amount < 0 ? 0 - static_cast<std::uint64_t>(amount) : static_cast<std::uint64_t>(amount); // 0 - x, not -x: MSVC warns (C4146)
  return std::format("{}{}.{:02}", amount < 0 ? "-" : "", magnitude / 100, magnitude % 100);
}

Amount scaled(Amount value, Amount by, Amount per) {
  auto const magnitude = [](Amount amount) {
    return amount < 0 ? 0 - static_cast<std::uint64_t>(amount) : static_cast<std::uint64_t>(amount);
  };
  auto const a = magnitude(value);
  auto const b = magnitude(by);
  auto const c = magnitude(per);
  // a * b = q * c + r, built one bit of b at a time so that r stays below c
  std::uint64_t q = 0;
  std::uint64_t r = 0;
  for (int bit = 63; bit >= 0; --bit) {
    q <<= 1;
    r <<= 1;
    if (r >= c) {
      r -= c;
      ++q;
    }
    if ((b >> bit) & 1) {
      q += a / c;
      r += a % c;
      if (r >= c) {
        r -= c;
        ++q;
      }
    }
  }
  if (r >= c - r) {
    ++q;
  }
  bool const negative = ((value < 0) != (by < 0)) != (per < 0);
  return negative ? -static_cast<Amount>(q) : static_cast<Amount>(q);
}

} // namespace cratchit
