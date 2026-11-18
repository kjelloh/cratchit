#include "core/csv.hpp"

#include <format>
#include <fstream>
#include <sstream>

namespace cratchit {

std::string format_csv_row(CsvRow const& row) {
  std::string line;
  for (std::size_t i = 0; i < row.size(); ++i) {
    if (i > 0) {
      line += ',';
    }
    auto const& field = row[i];
    bool const quoted = field.find_first_of(",\"\r\n") != std::string::npos ||
                        (!field.empty() && (field.front() == ' ' || field.back() == ' '));
    if (!quoted) {
      line += field;
      continue;
    }
    line += '"';
    for (char const c : field) {
      if (c == '"') {
        line += '"';
      }
      line += c;
    }
    line += '"';
  }
  line += '\n';
  return line;
}

std::expected<std::vector<CsvRow>, std::string> parse_csv(std::string_view text, char separator) {
  std::vector<CsvRow> rows;
  CsvRow row;
  std::string field;
  bool in_quotes = false;
  bool after_quote = false;
  bool row_has_content = false;
  std::size_t line = 1;
  auto end_row = [&] {
    if (row_has_content) {
      row.push_back(std::move(field));
      rows.push_back(std::move(row));
    }
    row = {};
    field.clear();
    row_has_content = false;
    after_quote = false;
  };
  for (std::size_t i = 0; i < text.size(); ++i) {
    char const c = text[i];
    if (in_quotes) {
      if (c == '"' && i + 1 < text.size() && text[i + 1] == '"') {
        field += '"';
        ++i;
      } else if (c == '"') {
        in_quotes = false;
        after_quote = true;
      } else {
        if (c == '\n') {
          ++line;
        }
        field += c;
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
      row.push_back(std::move(field));
      field.clear();
      row_has_content = true;
      after_quote = false;
      continue;
    }
    if (after_quote) {
      return std::unexpected(std::format("line {}: text after a closing quote", line));
    }
    row_has_content = true;
    if (c == '"' && field.empty()) {
      in_quotes = true;
    } else {
      field += c;
    }
  }
  if (in_quotes) {
    return std::unexpected(std::format("line {}: quote not closed", line));
  }
  end_row();
  return rows;
}

std::expected<std::vector<CsvRow>, std::string> read_csv(std::filesystem::path const& file) {
  std::ifstream in(file, std::ios::binary);
  if (!in) {
    return std::unexpected(std::format("Cannot read {}", file.string()));
  }
  std::ostringstream text;
  text << in.rdbuf();
  auto rows = parse_csv(text.str());
  if (!rows) {
    return std::unexpected(std::format("{}: {}", file.string(), rows.error()));
  }
  return rows;
}

std::expected<void, std::string> write_csv(std::filesystem::path const& file, std::vector<CsvRow> const& rows) {
  std::ofstream out(file, std::ios::binary | std::ios::trunc);
  if (!out) {
    return std::unexpected(std::format("Cannot write {}", file.string()));
  }
  for (auto const& row : rows) {
    out << format_csv_row(row);
  }
  if (!out) {
    return std::unexpected(std::format("Cannot write {}", file.string()));
  }
  return {};
}

} // namespace cratchit
