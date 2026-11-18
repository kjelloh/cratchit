#include "core/ar/statement.hpp"

#include <algorithm>
#include <format>
#include <set>

namespace cratchit::ar {

namespace {

// The index of the heading that starts the scope of the sum at index, or lines.size() when none
std::size_t scope_start(std::vector<StatementLine> const& lines, std::size_t index) {
  for (std::size_t at = index; at-- > 0;) {
    if (lines[at].kind == LineKind::heading && lines[at].level <= lines[index].level) {
      return at;
    }
  }
  return lines.size();
}

// The index after the last line under the heading at index
std::size_t scope_end(std::vector<StatementLine> const& lines, std::size_t index) {
  auto at = index + 1;
  while (at < lines.size() && !(lines[at].kind != LineKind::item && lines[at].level <= lines[index].level)) {
    ++at;
  }
  return at;
}

} // namespace

Statement build_statement(std::vector<Line> const& layout, std::vector<Balances> const& years) {
  Statement statement;
  // Whether each line of the layout is under the statement heading Noter
  std::vector<bool> in_notes;
  bool notes = false;
  for (auto const& line : layout) {
    if (line.kind == LineKind::heading && line.level == 1) {
      notes = line.text == "Noter";
    }
    in_notes.push_back(notes && line.kind != LineKind::outside);
  }
  // Every account with an amount must be on exactly one line of the resultaträkning and balansräkning,
  // and may be on one line of the notes
  std::set<std::string> checked;
  for (auto const& balances : years) {
    for (auto const& [account, amount] : balances) {
      if (amount == 0 || !checked.insert(account).second) {
        continue;
      }
      std::vector<std::string> texts;
      std::vector<std::string> note_texts;
      for (std::size_t index = 0; index < layout.size(); ++index) {
        auto const& line = layout[index];
        if ((line.kind == LineKind::item || line.kind == LineKind::outside) && line.accounts.contains(account)) {
          (in_notes[index] ? note_texts : texts).push_back(line.text);
        }
      }
      if (texts.empty()) {
        statement.errors.push_back(std::format("Account {} has an amount but is on no line of the årsredovisning", account));
      } else if (texts.size() > 1) {
        statement.errors.push_back(std::format("Account {} is on more than one line: {} and {}", account, texts[0], texts[1]));
      }
      if (note_texts.size() > 1) {
        statement.errors.push_back(std::format("Account {} is on more than one line of the notes: {} and {}", account,
                                               note_texts[0], note_texts[1]));
      }
    }
  }
  for (std::size_t index = 0; index < layout.size(); ++index) {
    auto const& line = layout[index];
    if (line.kind == LineKind::outside) {
      continue;
    }
    StatementLine out{line.level, line.kind, line.text, std::vector<Amount>(years.size(), 0), {}, {}, false, {}, in_notes[index]};
    if (line.kind == LineKind::item) {
      out.account_set = line.accounts;
      out.accounts.resize(years.size());
      for (std::size_t year = 0; year < years.size(); ++year) {
        for (auto const& [account, amount] : years[year]) {
          if (amount != 0 && line.accounts.contains(account)) {
            out.accounts[year][account] = amount;
            out.exact[year] += account.starts_with('1') ? amount : -amount;
            out.shown = true;
          }
        }
      }
    }
    statement.lines.push_back(std::move(out));
  }
  auto& lines = statement.lines;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    if (lines[index].kind == LineKind::sum) {
      for (auto at = scope_start(lines, index) + 1; at < index; ++at) {
        if (lines[at].kind == LineKind::item) {
          lines[index].shown = lines[index].shown || lines[at].shown;
          for (std::size_t year = 0; year < years.size(); ++year) {
            lines[index].exact[year] += lines[at].exact[year];
          }
        }
      }
    } else if (lines[index].kind == LineKind::heading) {
      auto const end = scope_end(lines, index);
      lines[index].shown = std::any_of(lines.begin() + static_cast<std::ptrdiff_t>(index) + 1,
                                       lines.begin() + static_cast<std::ptrdiff_t>(end),
                                       [](StatementLine const& line) { return line.kind == LineKind::item && line.shown; });
    }
  }
  for (auto& line : lines) {
    for (auto const amount : line.exact) {
      line.amounts.push_back(to_kronor(amount));
    }
  }
  return statement;
}

std::vector<Amount> const* amounts_of(Statement const& statement, std::string_view text) {
  auto const found = std::ranges::find(statement.lines, text, &StatementLine::text);
  return found == statement.lines.end() ? nullptr : &found->exact;
}

std::vector<std::string> check_statement(Statement const& statement) {
  std::vector<std::string> findings;
  auto const differ = [&](std::string_view label, std::vector<Amount> const& left, std::vector<Amount> const& right) {
    for (std::size_t year = 0; year < left.size() && year < right.size(); ++year) {
      if (left[year] != right[year]) {
        findings.push_back(std::format("Year {}: {} differ by {} ({} against {})", -static_cast<int>(year), label,
                                       format_amount(left[year] - right[year]), format_amount(left[year]),
                                       format_amount(right[year])));
      }
    }
  };
  auto const assets = amounts_of(statement, "Summa tillgångar");
  auto const equity_and_debt = amounts_of(statement, "Summa eget kapital och skulder");
  if (assets && equity_and_debt) {
    differ("Summa tillgångar and Summa eget kapital och skulder", *assets, *equity_and_debt);
  }
  // Årets resultat is first a sum in the resultaträkning, then an item in the balansräkning
  auto const first = std::ranges::find(statement.lines, "Årets resultat", &StatementLine::text);
  if (first != statement.lines.end()) {
    auto const second = std::find_if(first + 1, statement.lines.end(),
                                     [](StatementLine const& line) { return line.text == "Årets resultat"; });
    if (second != statement.lines.end()) {
      differ("Årets resultat in resultaträkning and balansräkning", first->exact, second->exact);
    }
  }
  return findings;
}

} // namespace cratchit::ar
