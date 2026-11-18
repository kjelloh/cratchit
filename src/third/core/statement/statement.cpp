#include "core/statement/statement.hpp"

#include "charset/detect.hpp"
#include "charset/utf8.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <map>
#include <ranges>

namespace cratchit::statement {

namespace {

bool is_digit(char c) {
  return c >= '0' && c <= '9';
}

std::string trimmed(std::string_view text) {
  auto const first = text.find_first_not_of(" \t");
  if (first == std::string_view::npos) {
    return {};
  }
  auto const last = text.find_last_not_of(" \t");
  return std::string(text.substr(first, last - first + 1));
}

// Lines as a short list, e.g. "Line 4", "Lines 1-3 and 9"
std::string lines_text(std::vector<std::size_t> const& lines) {
  std::vector<std::string> parts;
  for (std::size_t i = 0; i < lines.size();) {
    auto j = i;
    while (j + 1 < lines.size() && lines[j + 1] == lines[j] + 1) {
      ++j;
    }
    parts.push_back(i == j ? std::to_string(lines[i]) : std::format("{}–{}", lines[i], lines[j]));
    i = j + 1;
  }
  std::string text = lines.size() == 1 ? "Rad " : "Raderna ";
  for (std::size_t i = 0; i < parts.size(); ++i) {
    text += (i == 0 ? "" : i + 1 == parts.size() ? " och " : ", ") + parts[i];
  }
  return text;
}

// Column numbers from 1, e.g. "2 and 9"
std::string columns_text(std::vector<std::size_t> const& columns) {
  std::string text;
  for (std::size_t i = 0; i < columns.size(); ++i) {
    text += (i == 0 ? "" : i + 1 == columns.size() ? " och " : ", ") + std::to_string(columns[i] + 1);
  }
  return text;
}

bool is_entry_row(Row const& row) {
  auto const dates = std::ranges::count_if(row.cells, [](std::string const& cell) { return date_of(cell).has_value(); });
  return dates == 1 && std::ranges::any_of(row.cells, [](std::string const& cell) { return amount_of(cell).has_value(); });
}

std::size_t date_column_of(Row const& row) {
  return static_cast<std::size_t>(
      std::ranges::find_if(row.cells, [](std::string const& cell) { return date_of(cell).has_value(); }) -
      row.cells.begin());
}

// The saldo walk over rows oldest first: whether each saldo is the one before plus the amounts since,
// and the saldo before the first row and after the last
struct Walk {
  bool adds_up = true;
  std::optional<Amount> opening;
  std::optional<Amount> closing;
};

Walk walk(std::vector<Row const*> const& oldest_first, std::size_t amount_column, std::size_t saldo_column) {
  Walk result;
  std::optional<Amount> balance;
  Amount before = 0; // the amounts up to the first saldo
  for (auto const* row : oldest_first) {
    auto const amount = *amount_of(row->cells[amount_column]);
    auto const saldo = amount_of(row->cells[saldo_column]);
    if (balance) {
      *balance += amount;
      if (saldo && *saldo != *balance) {
        result.adds_up = false;
        return result;
      }
    } else {
      before += amount;
    }
    if (saldo) {
      if (!balance) {
        result.opening = *saldo - before;
      }
      balance = saldo;
    }
  }
  result.closing = balance;
  return result;
}

// Whether the dates of the rows, oldest first, never go back
bool dates_rise(std::vector<Row const*> const& rows, std::size_t date_column) {
  for (std::size_t i = 1; i < rows.size(); ++i) {
    if (*date_of(rows[i]->cells[date_column]) < *date_of(rows[i - 1]->cells[date_column])) {
      return false;
    }
  }
  return true;
}

} // namespace

std::expected<Decoded, std::string> decode(std::span<std::uint8_t const> bytes) {
  constexpr std::array<std::uint8_t, 3> bom{0xEF, 0xBB, 0xBF};
  bool const has_bom = bytes.size() >= bom.size() && std::ranges::equal(bytes.first(bom.size()), bom);
  if (has_bom) bytes = bytes.subspan(bom.size());
  Decoded result;
  if (has_bom || charset::detect::is_utf8(bytes)) {
    result.encoding = has_bom ? "UTF-8 med BOM" : "UTF-8";
    charset::utf8::Decoder from_utf8;
    charset::utf8::Encoder to_utf8;
    std::size_t line = 1;
    auto const add = [&](std::optional<charset::unicode::CodePoint> code_point) -> bool {
      if (!code_point) return true;
      if (from_utf8.failure_count() > 0) return false;
      line += *code_point == U'\n' ? 1 : 0;
      for (auto const byte : to_utf8.push(*code_point)) result.text += static_cast<char>(byte);
      return true;
    };
    for (auto const byte : bytes) {
      if (!add(from_utf8.push(byte))) {
        return std::unexpected(std::format("Rad {} är inte UTF-8, fast resten av filen är det.", line));
      }
    }
    if (!add(from_utf8.finish())) {
      return std::unexpected(std::format("Rad {} är inte UTF-8, fast resten av filen är det.", line));
    }
    return result;
  }
  // No encoding detected: ASCII, the bytes all text needs
  result.encoding = "ASCII";
  std::size_t line = 1;
  for (auto const byte : bytes) {
    if (byte >= 0x80) {
      return std::unexpected(std::format("cratchit känner inte igen filens teckenkodning: rad {} har tecken som inte "
                                         "är ASCII, och filen är inte UTF-8. Spara filen som UTF-8.",
                                         line));
    }
    line += byte == '\n' ? 1 : 0;
    result.text += static_cast<char>(byte);
  }
  return result;
}

std::vector<Row> split(std::string_view text, char separator) {
  std::vector<Row> rows;
  Row row{1, {}};
  std::string cell;
  bool quoted = false;      // inside quotes
  bool has_cell = false;    // the line has a cell started (text, quotes or a separator before)
  bool ended_by_separator = false;
  bool all_end_by_separator = true; // every line that is not blank
  std::size_t line = 1;
  auto const end_cell = [&] {
    row.cells.push_back(trimmed(cell));
    cell.clear();
  };
  auto const end_row = [&] {
    if (has_cell) {
      end_cell();
      all_end_by_separator = all_end_by_separator && ended_by_separator;
    }
    rows.push_back(std::move(row));
    row = Row{line + 1, {}};
    cell.clear();
    has_cell = false;
    ended_by_separator = false;
  };
  for (std::size_t i = 0; i < text.size(); ++i) {
    char const c = text[i];
    if (quoted) {
      if (c == '"' && i + 1 < text.size() && text[i + 1] == '"') {
        cell += '"';
        ++i;
      } else if (c == '"') {
        quoted = false;
      } else {
        line += c == '\n' ? 1 : 0;
        cell += c;
      }
      continue;
    }
    if (c == '\r' && i + 1 < text.size() && text[i + 1] == '\n') {
      continue;
    }
    if (c == '\n') {
      end_row();
      ++line;
      continue;
    }
    if (c == separator) {
      end_cell();
      has_cell = true;
      ended_by_separator = true;
      continue;
    }
    if (c == '"' && trimmed(cell).empty()) {
      cell.clear();
      quoted = true;
    } else {
      cell += c;
    }
    has_cell = true;
    ended_by_separator = false;
  }
  if (has_cell || !cell.empty()) {
    end_row();
  }
  if (all_end_by_separator) {
    for (auto& each : rows) {
      if (!each.cells.empty()) {
        each.cells.pop_back();
      }
    }
  }
  return rows;
}

std::optional<char> find_separator(std::string_view text) {
  std::optional<char> best;
  std::size_t best_count = 0;
  for (char const separator : {';', '\t', ','}) {
    std::map<std::size_t, std::size_t> rows_by_size;
    for (auto const& row : split(text, separator)) {
      if (row.cells.size() >= 3) {
        ++rows_by_size[row.cells.size()];
      }
    }
    for (auto const& [size, count] : rows_by_size) {
      if (count > best_count) {
        best_count = count;
        best = separator;
      }
    }
  }
  return best;
}

std::optional<Date> date_of(std::string_view cell) {
  if (cell.size() != 10 || (cell[4] != '-' && cell[4] != '/') || cell[7] != cell[4]) {
    return std::nullopt;
  }
  std::string iso(cell);
  iso[4] = iso[7] = '-';
  return parse_iso_date(iso);
}

std::optional<Amount> amount_of(std::string_view cell) {
  bool const negative = cell.starts_with('-');
  if (negative) {
    cell.remove_prefix(1);
  }
  // Digits, in groups of three after a space, U+00A0 or U+202F
  std::string whole;
  std::size_t group = 0; // digits since the last space
  bool grouped = false;
  std::size_t i = 0;
  while (i < cell.size()) {
    auto const rest = cell.substr(i);
    std::size_t space = rest.starts_with(' ') ? 1 : rest.starts_with("\xC2\xA0") ? 2 : rest.starts_with("\xE2\x80\xAF") ? 3 : 0;
    if (space > 0) {
      if (group == 0 || (grouped && group != 3) || (!grouped && group > 3)) {
        return std::nullopt;
      }
      grouped = true;
      group = 0;
      i += space;
    } else if (is_digit(cell[i])) {
      whole += cell[i];
      ++group;
      ++i;
    } else {
      break;
    }
  }
  if (group == 0 || (grouped && group != 3) || whole.size() > 15) {
    return std::nullopt;
  }
  std::string_view decimals;
  if (i < cell.size()) {
    if (cell[i] != ',' && cell[i] != '.') {
      return std::nullopt;
    }
    decimals = cell.substr(i + 1);
    if (decimals.empty() || decimals.size() > 2 || !std::ranges::all_of(decimals, is_digit)) {
      return std::nullopt;
    }
  }
  return parse_amount(std::format("{}{}{}{}", negative ? "-" : "", whole, decimals.empty() ? "" : ".", decimals));
}

std::expected<Document, std::string> parse_statement(std::span<std::uint8_t const> bytes) {
  auto const decoded = decode(bytes);
  if (!decoded) {
    return std::unexpected(decoded.error());
  }
  auto const text = &decoded->text;
  auto const separator = find_separator(*text);
  if (!separator) {
    return std::unexpected("Inga kolumner: ingen rad har minst tre celler åtskilda med ; tabb eller ,");
  }
  auto const rows = split(*text, *separator);

  // The runs of entry rows of the same shape
  struct Run {
    std::size_t first = 0; // index in rows
    std::size_t size = 0;
  };
  std::vector<Run> runs;
  for (std::size_t i = 0; i < rows.size(); ++i) {
    if (!is_entry_row(rows[i])) {
      continue;
    }
    auto const& last = runs.empty() ? Run{} : runs.back();
    auto const& before = rows[last.first];
    if (!runs.empty() && last.first + last.size == i && before.cells.size() == rows[i].cells.size() &&
        date_column_of(before) == date_column_of(rows[i])) {
      ++runs.back().size;
    } else {
      runs.push_back({i, 1});
    }
  }
  if (runs.empty()) {
    return std::unexpected("Inga rader att läsa: ingen rad har exakt ett datum och ett belopp");
  }
  std::ranges::stable_sort(runs, [](Run const& a, Run const& b) { return a.size > b.size; });
  auto const line_range = [&](Run const& run) {
    return std::format("{}–{}", rows[run.first].line, rows[run.first + run.size - 1].line);
  };
  if (runs.size() > 1 && runs[0].size == runs[1].size) {
    return std::unexpected(std::format("Två följder med {} rader (raderna {} och {}): cratchit kan inte avgöra vilka som är kontoutdragets rader",
                                       runs[0].size, line_range(runs[0]), line_range(runs[1])));
  }
  auto const run = runs.front();
  std::vector<Row const*> listed;
  for (std::size_t i = run.first; i < run.first + run.size; ++i) {
    listed.push_back(&rows[i]);
  }

  Document document;
  document.separator = *separator;
  document.encoding = decoded->encoding;
  document.date_column = date_column_of(*listed.front());
  document.first_line = listed.front()->line;
  document.last_line = listed.back()->line;
  auto const columns = listed.front()->cells.size();

  // Amount columns have an amount on every row; saldo columns an amount or nothing, and at least two amounts
  std::vector<std::size_t> amount_columns;
  std::vector<std::size_t> saldo_columns;
  for (std::size_t c = 0; c < columns; ++c) {
    if (c == document.date_column) {
      continue;
    }
    std::size_t amounts = 0;
    bool amount_or_empty = true;
    for (auto const* row : listed) {
      auto const& cell = row->cells[c];
      amounts += amount_of(cell) ? 1 : 0;
      amount_or_empty = amount_or_empty && (cell.empty() || amount_of(cell));
    }
    if (amounts == listed.size()) {
      amount_columns.push_back(c);
    }
    if (amount_or_empty && amounts >= 2) {
      saldo_columns.push_back(c);
    }
  }
  auto const first_line_text = std::format("rad {}", document.first_line);
  if (listed.size() == 1) {
    auto const amounts = std::ranges::count_if(listed.front()->cells, [](std::string const& cell) {
      return amount_of(cell).has_value();
    });
    if (amounts != 1) {
      return std::unexpected(std::format("Bara en rad ({}), med {} belopp: vilket som är beloppet och vilket som är "
                                         "saldot går inte att avgöra från en rad",
                                         first_line_text, amounts));
    }
  }

  std::vector<Row const*> reversed(listed.rbegin(), listed.rend());
  struct Found {
    std::size_t amount_column;
    std::size_t saldo_column;
    Order order;
    Walk walk;
  };
  std::vector<Found> found;
  for (auto const amount_column : amount_columns) {
    for (auto const saldo_column : saldo_columns) {
      if (saldo_column == amount_column) {
        continue;
      }
      for (auto const order : {Order::oldest_first, Order::newest_first}) {
        auto const result = walk(order == Order::oldest_first ? listed : reversed, amount_column, saldo_column);
        if (result.adds_up) {
          found.push_back({amount_column, saldo_column, order, result});
        }
      }
    }
  }
  if (found.size() > 1) {
    return std::unexpected(std::format("Raderna {}–{}: mer än ett par av belopp och saldo stämmer (beloppskolumner "
                                       "{}, saldokolumner {})",
                                       document.first_line, document.last_line, columns_text(amount_columns),
                                       columns_text(saldo_columns)));
  }
  if (found.size() == 1) {
    auto const& pair = found.front();
    document.amount_column = pair.amount_column;
    document.saldo_column = pair.saldo_column;
    document.order = pair.order;
    document.opening = pair.walk.opening;
    document.closing = pair.walk.closing;
    if (!dates_rise(pair.order == Order::oldest_first ? listed : reversed, document.date_column)) {
      return std::unexpected(std::format("Raderna {}–{}: datumen går emot ordningen som saldot visar ({})",
                                         document.first_line, document.last_line,
                                         pair.order == Order::oldest_first ? "äldst först" : "nyast först"));
    }
  } else if (amount_columns.size() == 1 &&
             std::ranges::all_of(saldo_columns, [&](std::size_t c) { return c == amount_columns.front(); })) {
    document.amount_column = amount_columns.front();
    bool const rise = dates_rise(listed, document.date_column);
    bool const fall = dates_rise(reversed, document.date_column);
    if (!rise && !fall) {
      return std::unexpected(std::format("Raderna {}–{}: datumen varken stiger eller faller", document.first_line,
                                         document.last_line));
    }
    document.order = rise && fall ? Order::unknown : rise ? Order::oldest_first : Order::newest_first;
    document.notes.push_back("Inget saldo: beloppen är inte kontrollerade.");
  } else if (amount_columns.size() == 1) {
    std::erase(saldo_columns, amount_columns.front());
    return std::unexpected(std::format("Raderna {}–{}: saldot stämmer inte (beloppskolumn {}, möjliga "
                                       "saldokolumner {})",
                                       document.first_line, document.last_line, amount_columns.front() + 1,
                                       columns_text(saldo_columns)));
  } else {
    return std::unexpected(std::format("Raderna {}–{}: kolumnerna {} har belopp på varje rad, och inget saldo stämmer "
                                       "som visar vilken som är beloppet",
                                       document.first_line, document.last_line, columns_text(amount_columns)));
  }

  // The text: the other columns, leaving out those the same on every row
  for (std::size_t c = 0; c < columns; ++c) {
    if (c == document.date_column || c == document.amount_column || c == document.saldo_column) {
      continue;
    }
    bool const same = std::ranges::all_of(listed, [&](Row const* row) { return row->cells[c] == listed.front()->cells[c]; });
    if (listed.size() < 2 || !same) {
      document.text_columns.push_back(c);
    }
  }
  auto const& oldest_first = document.order == Order::newest_first ? reversed : listed;
  for (auto const* row : oldest_first) {
    Entry entry;
    entry.date = *date_of(row->cells[document.date_column]);
    entry.amount = *amount_of(row->cells[document.amount_column]);
    if (document.saldo_column) {
      entry.saldo = amount_of(row->cells[*document.saldo_column]);
    }
    std::string const* before = nullptr;
    for (auto const c : document.text_columns) {
      auto const& cell = row->cells[c];
      if (cell.empty() || (before && *before == cell)) {
        continue;
      }
      entry.text += (entry.text.empty() ? "" : " | ") + cell;
      before = &cell;
    }
    entry.line = row->line;
    document.entries.push_back(std::move(entry));
  }

  std::vector<std::size_t> other_lines;
  for (auto const& row : rows) {
    bool const in_run = row.line >= document.first_line && row.line <= document.last_line;
    if (!in_run && std::ranges::any_of(row.cells, [](std::string const& cell) { return !cell.empty(); })) {
      other_lines.push_back(row.line);
    }
  }
  if (!other_lines.empty()) {
    document.notes.insert(document.notes.begin(),
                          lines_text(other_lines) + (other_lines.size() == 1 ? " är ingen rad i kontoutdraget." : " är inga rader i kontoutdraget."));
  }
  return document;
}

} // namespace cratchit::statement
