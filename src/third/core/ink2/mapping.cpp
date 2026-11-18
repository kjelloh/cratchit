#include "core/ink2/mapping.hpp"

#include "core/csv.hpp"

#include <algorithm>
#include <format>
#include <optional>

namespace cratchit::ink2 {

namespace {

// 1 or -1 by the SRU code (see Field::sign), nothing for a code not of INK2R or INK2S
std::optional<int> sign_of(std::string_view sru) {
  if (sru.size() != 4 || !std::ranges::all_of(sru, [](char c) { return c >= '0' && c <= '9'; })) {
    return std::nullopt;
  }
  auto const kind = sru.substr(0, 2);
  if (kind == "72" || kind == "75" || kind == "76") {
    return 1;
  }
  if (kind == "73" || kind == "74" || kind == "77") {
    return -1;
  }
  return std::nullopt;
}

} // namespace

std::string_view form_of(std::string_view sru) {
  if (sru.size() != 4) {
    return "";
  }
  auto const kind = sru.substr(0, 2);
  if (kind == "72" || kind == "73" || kind == "74" || kind == "75") {
    return "INK2R";
  }
  if (kind == "76" || kind == "77") {
    return "INK2S";
  }
  return "";
}

std::expected<std::vector<Line>, std::string> parse_mapping(std::string_view csv_text) {
  // A UTF-8 byte order mark is not part of the header
  if (csv_text.starts_with("\xEF\xBB\xBF")) {
    csv_text.remove_prefix(3);
  }
  auto const rows = parse_csv(csv_text, ';');
  if (!rows) {
    return std::unexpected(rows.error());
  }
  auto const extra_empty = [](CsvRow const& row) {
    return std::all_of(row.begin() + std::min<std::ptrdiff_t>(4, static_cast<std::ptrdiff_t>(row.size())), row.end(),
                       [](std::string const& field) { return field.empty(); });
  };
  if (rows->empty() || rows->front().size() < 4 || !std::equal(rows->front().begin(), rows->front().begin() + 4,
                                                                 CsvRow{"bas", "sru", "ink2", "text"}.begin()) ||
      !extra_empty(rows->front())) {
    return std::unexpected("The mapping does not start with the header bas;sru;ink2;text");
  }
  std::vector<Line> lines;
  for (std::size_t index = 1; index < rows->size(); ++index) {
    auto row = (*rows)[index];
    auto const row_error = [index](std::string_view what) {
      return std::unexpected(std::format("Mapping row {}: {}", index + 1, what));
    };
    if (!extra_empty(row)) {
      return row_error("text after the fourth column");
    }
    row.resize(4);
    auto const& [bas, sru, ink2, text] = std::tie(row[0], row[1], row[2], row[3]);
    if (bas.empty() && sru.empty() && ink2.empty()) {
      if (!text.empty()) {
        lines.push_back({Line::Kind::heading, "", "", text, {}});
      }
      continue;
    }
    if (bas.empty() || sru.empty() || ink2.empty() != text.empty()) {
      return row_error("a row of the form needs bas, sru, ink2 and text; a second field bas and sru only");
    }
    auto accounts = parse_account_set(bas);
    if (!accounts) {
      return row_error(accounts.error());
    }
    auto sign = sign_of(sru);
    if (!sign) {
      return row_error(std::format("\"{}\" is not an SRU code of INK2R or INK2S (72xx-77xx)", sru));
    }
    auto const& groups = accounts->groups;
    if (std::ranges::any_of(groups, [](AccountGroup const& group) { return group.condition == Condition::profit; })) {
      sign = 1;
    } else if (std::ranges::any_of(groups, [](AccountGroup const& group) { return group.condition == Condition::loss; })) {
      sign = -1;
    }
    Field field{sru, bas, std::move(*accounts), *sign};
    if (ink2.empty()) {
      if (lines.empty() || lines.back().kind != Line::Kind::row) {
        return row_error("a second field without a row of the form before it");
      }
      if (form_of(field.sru) != lines.back().form) {
        return row_error("a second field of another form than the row before it");
      }
      lines.back().fields.push_back(std::move(field));
    } else {
      auto form = std::string(form_of(field.sru));
      lines.push_back({Line::Kind::row, std::move(form), ink2, text, {std::move(field)}});
    }
  }
  // A heading belongs to the form of the row after it
  std::string next_form;
  for (auto line = lines.rbegin(); line != lines.rend(); ++line) {
    if (line->kind == Line::Kind::row) {
      next_form = line->form;
    } else {
      line->form = next_form;
    }
  }
  return lines;
}

} // namespace cratchit::ink2
