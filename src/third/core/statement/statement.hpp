#pragma once

#include "core/amount.hpp"
#include "core/date.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// Account statements: the entries of one account (bank or tax account) as a
// file from the bank or SKV lists them, found by the content of the cells only.
// Nothing about the bank, the account or BAS accounts is taken from the file.
namespace cratchit::statement {

// The text of a statement file as UTF-8, and the encoding it was read in
struct Decoded {
  std::string text;
  std::string encoding; // "UTF-8 med BOM", "UTF-8" or "ASCII"
};

// The text of a statement file (from take 3, with detection added): UTF-8
// after a UTF-8 byte order mark, which is skipped; else UTF-8 when the bytes
// are (see charset::detect::is_utf8); else ASCII. Error text names the line
// that is not UTF-8, or the first line with a byte outside ASCII when no
// encoding is detected.
std::expected<Decoded, std::string> decode(std::span<std::uint8_t const> bytes);

// One line of a statement file split into cells, each trimmed of spaces and tabs
struct Row {
  std::size_t line = 0; // in the file, from 1
  std::vector<std::string> cells;

  bool operator==(Row const&) const = default;
};

// The rows of the text, cells separated by separator. A cell in double quotes
// may hold the separator and doubled quotes. CR before LF is dropped. A blank
// line is a row without cells. When every line that is not blank ends with the
// separator (as Nordea writes it), the empty cell after it is left out.
std::vector<Row> split(std::string_view text, char separator);

// The separator of ;, tab and , that gives the most rows with the same number
// of cells (at least 3), ; on a tie; none when no separator does
std::optional<char> find_separator(std::string_view text);

// A pure date cell: YYYY-MM-DD or YYYY/MM/DD and nothing else
std::optional<Date> date_of(std::string_view cell);

// A pure amount cell: an optional -, digits, optionally in groups of three
// after a space or no-break space (2 292), and optionally a decimal comma or
// point with one or two decimals (-1308,75). At most 15 digits before the decimals.
std::optional<Amount> amount_of(std::string_view cell);

// One entry of the statement: an amount into or out of the account
struct Entry {
  Date date;
  Amount amount = 0;           // into the account positive, out of it negative
  std::optional<Amount> saldo; // the account's balance after it, when the row has one
  std::string text;            // the row's other cells, see Document::text_columns
  std::size_t line = 0;        // in the file

  bool operator==(Entry const&) const = default;
};

// How the file lists the entries
enum class Order { oldest_first, newest_first, unknown };

// The entries of a statement file and how they were found. Columns count from 0.
struct Document {
  std::string encoding; // as decode read the file
  char separator = ';';
  std::size_t date_column = 0;
  std::size_t amount_column = 0;
  std::optional<std::size_t> saldo_column; // only when it adds up, row by row
  // The columns joined into the text, in order: all but the date, amount and
  // saldo, and (with two rows or more) those holding the same on every row
  std::vector<std::size_t> text_columns;
  Order order = Order::unknown;
  // The saldo before the first entry and after the last, when there is a saldo column
  std::optional<Amount> opening;
  std::optional<Amount> closing;
  std::vector<Entry> entries; // oldest first (as listed when the order is unknown)
  std::size_t first_line = 0;    // the range of entry rows in the file
  std::size_t last_line = 0;
  std::vector<std::string> notes; // worth knowing, e.g. the lines that are not entries
};

// Finds the entries of a statement file:
// - The rows are split by the separator found (see find_separator). A
//   entry row has exactly one date cell and at least one amount cell.
// - The entries are the longest run of consecutive entry rows with the
//   same number of cells and the date in the same column. Any other row ends
//   a run. Two longest runs are an error.
// - The amount is in a column with an amount on every row; a saldo column has
//   an amount or nothing on each row, and at least two amounts. Each pair of
//   them, and each order, is tried: the saldo must equal the saldo before plus
//   the amounts since, on every row that has one. Exactly one pair and order
//   must add up. With none, a single amount column and no possible saldo
//   column, there is no saldo; else it is an error.
// - The order is the saldo's; the dates must not go against it. Without a
//   saldo, the dates tell (unknown when they are all the same).
// - A single entry row must have exactly one amount (no saldo).
// - The text joins the row's text cells with ' | ', leaving out empty cells
//   and a cell equal to the one before.
// Error text names the lines. Does not log.
std::expected<Document, std::string> parse_statement(std::span<std::uint8_t const> bytes);

} // namespace cratchit::statement
