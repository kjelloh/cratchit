#pragma once

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit::ar {

// What the command line asks for
struct Arguments {
  bool help = false;                 // --help or -h, alone
  bool trace = false;                // --trace
  std::optional<int> bas_year;       // --bas-year <year>, else the default
  std::filesystem::path file;        // the SIE file, last
};

// cratchit --sie-to-ar [--trace] [--bas-year <year>] <file.se>, the options in any order,
// or cratchit --sie-to-ar --help (-h). Error text for an unknown option, a missing or
// non-numeric year, an option given twice, or a missing or extra file.
std::expected<Arguments, std::string> parse_arguments(std::vector<std::string_view> const& arguments);

// The usage line
constexpr std::string_view usage = "Usage: cratchit --sie-to-ar [--trace] [--bas-year <year>] <file.se>";

} // namespace cratchit::ar
