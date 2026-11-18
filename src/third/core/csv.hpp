#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit {

using CsvRow = std::vector<std::string>;

// One line of comma separated fields, ending with a newline. A field holding
// a comma, a quote, a CR or LF, or space at either end is quoted, with quotes doubled.
std::string format_csv_row(CsvRow const& row);

// The rows of CSV text, fields separated by separator (a comma unless given). Quoted fields may hold the
// separator, doubled quotes and line ends. A CR before LF is dropped and blank lines are skipped. Error text for
// an unclosed quote or text after a closing quote.
std::expected<std::vector<CsvRow>, std::string> parse_csv(std::string_view text, char separator = ',');

// Read and parse a file. Error text names the file.
std::expected<std::vector<CsvRow>, std::string> read_csv(std::filesystem::path const& file);

// Write rows to a file, replacing what it held. Error text names the file.
std::expected<void, std::string> write_csv(std::filesystem::path const& file, std::vector<CsvRow> const& rows);

} // namespace cratchit
