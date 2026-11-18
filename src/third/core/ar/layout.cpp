#include "core/ar/layout.hpp"

#include "core/csv.hpp"

#include <algorithm>
#include <charconv>
#include <format>
#include <optional>

namespace cratchit::ar {

namespace {

std::optional<int> account_number(std::string_view text) {
  if (text.size() != 4 || !std::ranges::all_of(text, [](char c) { return c >= '0' && c <= '9'; })) {
    return std::nullopt;
  }
  int number = 0;
  std::from_chars(text.data(), text.data() + text.size(), number);
  return number;
}

bool in_any(std::vector<std::pair<int, int>> const& ranges, int number) {
  return std::ranges::any_of(ranges, [number](auto const& range) { return range.first <= number && number <= range.second; });
}

// Reads the account set text left to right
class SetReader {
public:
  explicit SetReader(std::string_view text) : text_(text) {}

  std::expected<AccountSet, std::string> read() {
    AccountSet set;
    if (!read_list(set.included)) {
      return error();
    }
    while (skip_space(), !at_end()) {
      if (!take("(") || !take_word("förutom") || !read_list(set.excluded) || !take(")")) {
        return error();
      }
      // An exclusion may be followed by more of the set: "... (förutom 1573) eller 1670-1679"
      if (skip_space(), at_end()) {
        break;
      }
      if (!take_separator() || !read_list(set.included)) {
        return error();
      }
    }
    return set;
  }

private:
  std::unexpected<std::string> error() const {
    return std::unexpected(std::format("Unreadable BAS accounts \"{}\" at \"{}\"", text_, text_.substr(position_)));
  }

  bool at_end() const { return position_ >= text_.size(); }

  void skip_space() {
    while (!at_end() && text_[position_] == ' ') {
      ++position_;
    }
  }

  bool take(std::string_view what) {
    skip_space();
    if (!text_.substr(position_).starts_with(what)) {
      return false;
    }
    position_ += what.size();
    return true;
  }

  bool take_word(std::string_view word) {
    auto const start = position_;
    if (take(word) && (at_end() || text_[position_] == ' ')) {
      return true;
    }
    position_ = start;
    return false;
  }

  bool take_separator() { return take(",") || take_word("eller") || take_word("och"); }

  std::optional<int> take_number() {
    skip_space();
    auto const start = position_;
    while (!at_end() && text_[position_] >= '0' && text_[position_] <= '9') {
      ++position_;
    }
    return account_number(text_.substr(start, position_ - start));
  }

  // A number or range, then more after each separator; stops before "(" or ")"
  bool read_list(std::vector<std::pair<int, int>>& ranges) {
    while (true) {
      auto const first = take_number();
      if (!first) {
        return false;
      }
      auto last = first;
      if (take("-")) {
        last = take_number();
        if (!last || *last < *first) {
          return false;
        }
      }
      ranges.emplace_back(*first, *last);
      auto const before = position_;
      if (!take_separator()) {
        position_ = before;
        return true;
      }
    }
  }

  std::string_view text_;
  std::size_t position_ = 0;
};

std::optional<LineKind> line_kind(std::string_view text) {
  if (text == "Rubrik") {
    return LineKind::heading;
  }
  if (text == "Post") {
    return LineKind::item;
  }
  if (text == "Summa") {
    return LineKind::sum;
  }
  if (text == "Utanför") {
    return LineKind::outside;
  }
  return std::nullopt;
}

} // namespace

bool AccountSet::contains(std::string_view account) const {
  auto const number = account_number(account);
  return number && in_any(included, *number) && !in_any(excluded, *number);
}

std::optional<BuiltInLayout> layout_for(int bas_year) {
  std::optional<BuiltInLayout> chosen;
  for (auto const& layout : built_in_layouts()) {
    if (layout.year <= bas_year) {
      chosen = layout;
    }
  }
  return chosen;
}

std::string bas_years_of(BuiltInLayout const& layout) {
  auto const& layouts = built_in_layouts();
  auto const next = std::ranges::find_if(layouts, [&layout](BuiltInLayout const& other) { return other.year > layout.year; });
  if (next == layouts.end()) {
    return std::format("BAS {} and later", layout.year);
  }
  return next->year - 1 == layout.year ? std::format("BAS {}", layout.year)
                                       : std::format("BAS {}-{}", layout.year, next->year - 1);
}

std::string_view layout_text() {
  return layout_for(default_bas_year)->text;
}

std::expected<AccountSet, std::string> parse_account_set(std::string_view text) {
  return SetReader(text).read();
}

std::expected<std::vector<Line>, std::string> parse_layout(std::string_view csv_text) {
  auto const rows = parse_csv(csv_text, ';');
  if (!rows) {
    return std::unexpected(rows.error());
  }
  if (rows->empty() || *rows->begin() != CsvRow{"Nivå", "Typ", "BAS Range", "ÅR"}) {
    return std::unexpected("The mapping does not start with the header Nivå;Typ;BAS Range;ÅR");
  }
  std::vector<Line> lines;
  for (std::size_t index = 1; index < rows->size(); ++index) {
    auto const& row = (*rows)[index];
    auto const row_error = [index](std::string_view what) {
      return std::unexpected(std::format("Mapping row {}: {}", index + 1, what));
    };
    if (row.size() != 4) {
      return row_error("not 4 fields");
    }
    Line line;
    auto const kind = line_kind(row[1]);
    if (!kind) {
      return row_error(std::format("unknown Typ \"{}\"", row[1]));
    }
    line.kind = *kind;
    line.text = row[3];
    if (line.text.empty()) {
      return row_error("no ÅR text");
    }
    if (line.kind == LineKind::outside) {
      if (!row[0].empty()) {
        return row_error("Utanför has no Nivå");
      }
    } else if (row[0].size() != 1 || row[0][0] < '1' || row[0][0] > '9') {
      return row_error(std::format("Nivå \"{}\" is not 1-9", row[0]));
    } else {
      line.level = row[0][0] - '0';
    }
    bool const has_accounts = line.kind == LineKind::item || line.kind == LineKind::outside;
    if (has_accounts) {
      auto accounts = parse_account_set(row[2]);
      if (!accounts) {
        return row_error(accounts.error());
      }
      line.accounts = std::move(*accounts);
    } else if (!row[2].empty()) {
      return row_error("only Post and Utanför have BAS accounts");
    }
    lines.push_back(std::move(line));
  }
  return lines;
}

} // namespace cratchit::ar
