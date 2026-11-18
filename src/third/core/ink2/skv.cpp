#include "core/ink2/skv.hpp"

#include "core/csv.hpp"

#include <algorithm>
#include <iterator>
#include <format>

namespace cratchit::ink2 {

std::expected<std::vector<SkvField>, std::string> parse_skv_fields(std::string_view csv_text) {
  if (csv_text.starts_with("\xEF\xBB\xBF")) {
    csv_text.remove_prefix(3);
  }
  auto const rows = parse_csv(csv_text, ';');
  if (!rows) {
    return std::unexpected(rows.error());
  }
  if (rows->empty() || rows->front() != CsvRow{"form", "sru", "ink2", "text", "sign", "type"}) {
    return std::unexpected("The SKV fields do not start with the header form;sru;ink2;text;sign;type");
  }
  std::vector<SkvField> fields;
  for (std::size_t index = 1; index < rows->size(); ++index) {
    auto const& row = (*rows)[index];
    auto const row_error = [index](std::string_view what) {
      return std::unexpected(std::format("SKV fields row {}: {}", index + 1, what));
    };
    if (row.size() != 6) {
      return row_error("not 6 fields");
    }
    SkvField field{row[0], row[1], row[2], row[3], ' ', SkvField::Type::amount};
    if (field.form != "INK2" && field.form != "INK2R" && field.form != "INK2S") {
      return row_error(std::format("unknown form \"{}\"", field.form));
    }
    if (field.sru.empty() || !std::ranges::all_of(field.sru, [](char c) { return c >= '0' && c <= '9'; })) {
      return row_error(std::format("\"{}\" is not an SRU code", field.sru));
    }
    if (std::ranges::any_of(fields, [&](SkvField const& other) { return other.sru == field.sru; })) {
      return row_error(std::format("SRU code {} given twice", field.sru));
    }
    if (row[5] == "mark") {
      field.type = SkvField::Type::mark;
      if (!row[4].empty()) {
        return row_error("a mark has no sign");
      }
    } else if (row[5] == "amount") {
      if (row[4] != "+" && row[4] != "-" && row[4] != "*") {
        return row_error(std::format("sign \"{}\" is not +, - or *", row[4]));
      }
      field.sign = row[4][0];
    } else {
      return row_error(std::format("type \"{}\" is not amount or mark", row[5]));
    }
    fields.push_back(std::move(field));
  }
  return fields;
}

std::vector<SkvField> fields_of(std::vector<SkvField> const& fields, std::string_view form) {
  std::vector<SkvField> out;
  std::ranges::copy_if(fields, std::back_inserter(out), [form](SkvField const& field) { return field.form == form; });
  return out;
}

} // namespace cratchit::ink2
