#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit::ink2 {

// What the command line asks for
struct Arguments {
  bool help = false;                              // --help or -h, alone
  bool trace = false;                             // --trace: print the trace instead of writing the folder
  std::filesystem::path sie;                      // the SIE file of the year
  std::optional<std::filesystem::path> previous;  // last year's return: its BLANKETTER.SRU or the folder of it
  std::optional<std::int64_t> previous_deficit;   // --previous-deficit <kronor>
};

// cratchit --sie-to-ink2 [--trace] <file.se> <last year's .sru or folder>, or cratchit --sie-to-ink2 [--trace]
// <file.se> --previous-deficit <kronor>, in any order; or cratchit --sie-to-ink2 --help (-h). The SIE file is told by
// its ending (.se, .si or .sie, any case), last year's return by .sru or by being a folder.
// Error text for an unknown option, an option given twice, a file of neither kind, two of a kind, a missing SIE file,
// neither or both of last year's return and --previous-deficit, and a deficit that is not
// whole kronor of 0 or more.
std::expected<Arguments, std::string> parse_arguments(std::vector<std::string_view> const& arguments);

// The usage lines
constexpr std::string_view usage =
    "Usage: cratchit --sie-to-ink2 [--trace] <file.se> <last year's BLANKETTER.SRU or its folder>\n"
    "       cratchit --sie-to-ink2 [--trace] <file.se> --previous-deficit <kronor>";

} // namespace cratchit::ink2
