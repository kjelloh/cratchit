#include "core/ar/arguments.hpp"

#include <charconv>
#include <format>

namespace cratchit::ar {

std::expected<Arguments, std::string> parse_arguments(std::vector<std::string_view> const& arguments) {
  if (arguments.size() == 1 && (arguments.front() == "--help" || arguments.front() == "-h")) {
    return Arguments{true, false, std::nullopt, {}};
  }
  Arguments parsed;
  bool has_file = false;
  for (std::size_t index = 0; index < arguments.size(); ++index) {
    auto const argument = arguments[index];
    if (argument == "--trace") {
      if (parsed.trace) {
        return std::unexpected("--trace is given twice");
      }
      parsed.trace = true;
    } else if (argument == "--bas-year") {
      if (parsed.bas_year) {
        return std::unexpected("--bas-year is given twice");
      }
      if (index + 1 == arguments.size()) {
        return std::unexpected("--bas-year needs a year, e.g. --bas-year 2026");
      }
      auto const year = arguments[++index];
      int value = 0;
      auto const [end, error] = std::from_chars(year.data(), year.data() + year.size(), value);
      if (error != std::errc{} || end != year.data() + year.size() || year.size() != 4) {
        return std::unexpected(std::format("--bas-year needs a four digit year, not \"{}\"", year));
      }
      parsed.bas_year = value;
    } else if (argument.starts_with('-')) {
      return std::unexpected(std::format("Unknown option {}", argument));
    } else if (has_file) {
      return std::unexpected(std::format("Only one SIE file, not also {}", argument));
    } else {
      parsed.file = argument;
      has_file = true;
    }
  }
  if (!has_file) {
    return std::unexpected("No SIE file given");
  }
  return parsed;
}

} // namespace cratchit::ar
