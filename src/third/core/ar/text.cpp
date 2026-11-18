#include "core/ar/text.hpp"

#include "core/ar/requirements.hpp"
#include "core/date.hpp"

#include <map>
#include <optional>

#include <algorithm>

#include <format>

namespace cratchit::ar {

namespace {

constexpr std::size_t text_width = 56;
constexpr std::size_t column_width = 14;
constexpr std::size_t note_width = 6; // the Not column of the balansräkning, at the end of the text column


// Characters, not bytes, of UTF-8 text
std::size_t width_of(std::string_view text) {
  std::size_t width = 0;
  for (char const c : text) {
    width += (static_cast<unsigned char>(c) & 0xC0) != 0x80;
  }
  return width;
}

std::string padded(std::string_view text, std::size_t width) {
  auto const used = width_of(text);
  return std::string(text) + std::string(used < width ? width - used : 0, ' ');
}

std::string right(std::string_view text, std::size_t width) {
  auto const used = width_of(text);
  return std::string(used < width ? width - used : 0, ' ') + std::string(text);
}

// The text broken at spaces into lines of at most width characters, where it can be
std::vector<std::string> wrapped(std::string_view text, std::size_t width) {
  std::vector<std::string> lines(1);
  while (!text.empty()) {
    auto const space = text.find(' ');
    auto const word = text.substr(0, space);
    if (!lines.back().empty() && width_of(lines.back()) + 1 + width_of(word) > width) {
      lines.emplace_back();
    }
    if (!lines.back().empty()) {
      lines.back() += ' ';
    }
    lines.back() += word;
    text = space == std::string_view::npos ? std::string_view{} : text.substr(space + 1);
  }
  return lines;
}

constexpr std::size_t paragraph_width = 84; // running text

// A title underlined with = (a part of the årsredovisning) or - (a section of the förvaltningsberättelse)
std::string titled(std::string_view title, char underline) {
  return std::format("{}\n{}\n", title, std::string(width_of(title), underline));
}

// Running text wrapped at paragraph_width, indented; lines after the first indented by hanging more
void add_paragraph(std::string& out, std::string_view text, std::size_t indent = 0, std::size_t hanging = 0) {
  auto const lines = wrapped(text, paragraph_width - indent - hanging);
  for (std::size_t index = 0; index < lines.size(); ++index) {
    out += std::string(indent + (index > 0 ? hanging : 0), ' ') + lines[index] + '\n';
  }
}

// The marker of a missing requirement, e.g. "TBD [4]"; "TBD" when the check does not list it
std::string marker_of(Check const& check, std::string_view id) {
  return check.marker(id).value_or("TBD");
}

// One row of text and amounts; the amounts on the last line of a wrapped text. With
// a note, the last note_width characters of the text column hold it, right aligned.
void add_row(std::string& out, std::string_view text, std::size_t indent, std::vector<std::string> const& columns,
             std::size_t width = text_width, std::optional<std::string_view> note = std::nullopt) {
  auto const text_end = note ? width - note_width : width;
  auto const lines = wrapped(text, text_end - indent);
  for (std::size_t index = 0; index < lines.size(); ++index) {
    out += std::string(indent, ' ');
    if (index + 1 < lines.size()) {
      out += lines[index] + '\n';
      continue;
    }
    if (columns.empty()) {
      out += lines[index] + '\n';
      continue;
    }
    auto row = padded(lines[index], text_end - indent);
    if (note) {
      row += right(*note, note_width);
    }
    for (auto const& column : columns) {
      row += right(column, column_width);
    }
    out += row.substr(0, row.find_last_not_of(' ') + 1) + '\n';
  }
}

// Headings and sums by level below the statement; items two more than the heading over them
std::size_t indent_of(StatementLine const& line) {
  return line.level > 1 ? 2 * static_cast<std::size_t>(line.level - 2) : 0;
}

std::vector<std::string> amount_columns(StatementLine const& line) {
  std::vector<std::string> columns;
  for (auto const amount : line.amounts) {
    columns.push_back(format_kronor(amount));
  }
  return columns;
}

// A statement's title and the years over its columns: periods for the
// resultaträkning, closing days for the balansräkning
void add_title(std::string& out, std::string_view title, std::vector<sie::FiscalYear> const& years,
               std::optional<std::string_view> note = std::nullopt, char underline = '=') {
  out += std::format("{}\n{}\n", title, std::string(width_of(title), underline));
  std::vector<std::string> first;
  std::vector<std::string> second;
  for (auto const& year : years) {
    if (title != "Balansräkning") {
      first.push_back(format_iso_date(year.start));
      second.push_back("-" + format_iso_date(year.end));
    } else {
      first.push_back(format_iso_date(year.end));
    }
  }
  add_row(out, "", 0, first, text_width, note);
  if (!second.empty()) {
    add_row(out, "", 0, second);
  }
}

} // namespace

std::string format_kronor(Kronor kronor) {
  auto const digits = std::to_string(kronor < 0 ? -kronor : kronor);
  std::string out = kronor < 0 ? "-" : "";
  for (std::size_t index = 0; index < digits.size(); ++index) {
    if (index > 0 && (digits.size() - index) % 3 == 0) {
      out += ' ';
    }
    out += digits[index];
  }
  return out;
}

std::string format_equity_changes(EquityChanges const& changes) {
  constexpr std::size_t width = 30; // narrower text, as the table has a column per post and the total
  std::string out = titled("Förändringar i eget kapital", '-');
  // Column titles wrapped over lines, bottom aligned
  std::vector<std::vector<std::string>> titles;
  for (auto const& column : changes.columns) {
    titles.push_back(wrapped(column, column_width - 2));
  }
  titles.push_back({"Totalt"});
  std::size_t height = 0;
  for (auto const& lines : titles) {
    height = std::max(height, lines.size());
  }
  for (std::size_t line = 0; line < height; ++line) {
    std::vector<std::string> header;
    for (auto const& lines : titles) {
      auto const skip = height - lines.size();
      header.push_back(line < skip ? "" : lines[line - skip]);
    }
    add_row(out, "", 0, header, width);
  }
  out += '\n';
  for (std::size_t index = 0; index < changes.rows.size(); ++index) {
    auto const& row = changes.rows[index];
    // A change leaves the columns it does not touch blank; the first and last row and the total show 0
    bool const is_change = index > 0 && index + 1 < changes.rows.size();
    std::vector<std::string> columns;
    for (std::size_t column = 0; column < row.amounts.size(); ++column) {
      auto const kronor = to_kronor(row.amounts[column]);
      bool const blank = is_change && kronor == 0 && column + 1 < row.amounts.size();
      columns.push_back(blank ? "" : format_kronor(kronor));
    }
    add_row(out, row.text, 0, columns, width);
  }
  return out;
}

std::string format_notes(Notes const& notes, std::vector<sie::FiscalYear> const& years, Check const& check) {
  std::string out = titled("Noter", '=');
  out += "\nNot 1 Redovisningsprinciper\n";
  add_paragraph(out, "Årsredovisningen är upprättad i enlighet med årsredovisningslagen och Bokföringsnämndens allmänna "
                     "råd (BFNAR 2016:10) om årsredovisning i mindre företag.",
                2);
  if (auto const missing = check.marker("principer")) {
    add_paragraph(out, std::format("Avskrivningstider: {}", *missing), 2);
  }
  if (auto const missing = check.marker("anstallda")) {
    out += "\nNot 2 Medelantal anställda\n";
    add_paragraph(out, std::format("Medelantal anställda under året: {}", *missing), 2);
  }
  for (auto const& note : notes.notes) {
    std::vector<std::string> const header{format_iso_date(years.front().end)};
    out += '\n';
    add_row(out, note.title, 0, header);
    for (auto const& section : note.sections) {
      out += '\n';
      add_row(out, section.text, 0, {});
      for (auto const& row : section.rows) {
        add_row(out, row.text, 2, {format_kronor(to_kronor(row.amount))});
      }
    }
    out += '\n';
    add_row(out, note.sum_text, 0, {format_kronor(to_kronor(note.sum))});
  }
  return out;
}

std::string format_statements(Statement const& statement, std::vector<sie::FiscalYear> const& years,
                              std::map<std::string, std::string> const& notes) {
  std::string out;
  std::size_t item_indent = 2;
  bool note_column = false; // in the balansräkning, when there are notes
  for (auto const& line : statement.lines) {
    if (line.in_notes) {
      break; // the notes are printed by format_notes
    }
    if (line.kind == LineKind::heading && line.level == 1) {
      if (!out.empty()) {
        out += '\n';
      }
      note_column = line.text == "Balansräkning" && !notes.empty();
      add_title(out, line.text, years, note_column ? std::optional<std::string_view>("Not") : std::nullopt);
      continue;
    }
    if (!line.shown) {
      continue;
    }
    std::optional<std::string_view> note;
    if (note_column) {
      auto const found = line.kind == LineKind::item ? notes.find(line.text) : notes.end();
      note = found == notes.end() ? std::string_view{} : std::string_view(found->second);
    }
    if (line.kind == LineKind::heading) {
      out += '\n';
      add_row(out, line.text, indent_of(line), {});
      item_indent = indent_of(line) + 2;
    } else if (line.kind == LineKind::sum && line.level == 1) {
      out += '\n';
      add_row(out, line.text, 0, amount_columns(line), text_width, note);
    } else {
      add_row(out, line.text, line.kind == LineKind::item ? item_indent : indent_of(line), amount_columns(line),
              text_width, note);
    }
  }
  return out;
}

std::string format_overview(Overview const& overview, std::vector<sie::FiscalYear> const& years, Check const& check) {
  std::string out;
  add_title(out, "Flerårsöversikt", years, std::nullopt, '-');
  out += '\n';
  std::vector<std::string> net_sales;
  std::vector<std::string> result;
  std::vector<std::string> solidity;
  for (std::size_t year = 0; year < years.size(); ++year) {
    net_sales.push_back(format_kronor(to_tkr(overview.net_sales[year])));
    result.push_back(format_kronor(to_tkr(overview.result_after_financial_items[year])));
    solidity.push_back(overview.solidity[year] ? format_percent(*overview.solidity[year]) : "-");
  }
  add_row(out, "Nettoomsättning (tkr)", 0, net_sales);
  add_row(out, "Resultat efter finansiella poster (tkr)", 0, result);
  add_row(out, "Soliditet (%)", 0, solidity);
  if (auto const missing = check.marker("flerar")) {
    out += '\n';
    add_row(out, "De två föregående räkenskapsåren", 0, {*missing});
  }
  if (auto const missing = check.marker("omsattning")) {
    out += '\n';
    add_paragraph(out, std::format("Kommentar till nettoomsättningens förändring: {}", *missing));
  }
  return out;
}

std::string format_disposition(Disposition const& disposition, Check const& check) {
  std::string out = titled("Resultatdisposition", '-') + '\n';
  auto const total = format_kronor(to_kronor(disposition.total));
  add_row(out, "Till årsstämmans förfogande står följande medel", 0, {});
  for (auto const& row : disposition.funds) {
    add_row(out, row.text, 2, {format_kronor(to_kronor(row.amount))});
  }
  add_row(out, "Summa", 0, {total});
  out += '\n';
  add_row(out, "Styrelsen föreslår att medlen disponeras enligt följande", 0, {});
  for (auto const& text : disposition.proposal) {
    add_row(out, text, 2, {marker_of(check, "forslag")});
  }
  add_row(out, "Summa", 0, {total});
  return out;
}

std::string format_front(sie::Document const& document, std::vector<sie::FiscalYear> const& years,
                         Check const& check) {
  std::string out = std::format("{}\n{}\n", document.company_name, document.orgnr);
  out += std::format("Årsredovisning för räkenskapsåret {} - {}\n\n", format_iso_date(years.front().start),
                     format_iso_date(years.front().end));
  if (auto const unclosed = check.marker("bokslut")) {
    add_paragraph(out, std::format("PRELIMINÄR: bokslutet är inte gjort, årets resultat är inte bokfört ({}).",
                                   *unclosed));
    out += '\n';
  }
  auto const currency = document.currency.empty() || document.currency == "SEK" ? std::string("svenska kronor (SEK)")
                                                                                 : document.currency;
  add_paragraph(out, std::format("Styrelsen avger följande årsredovisning. Redovisningsvalutan är {} och alla belopp "
                                 "anges i hela kronor om inte annat anges.",
                                 currency));
  return out;
}

std::string format_business(Check const& check) {
  std::string out = titled("Förvaltningsberättelse", '=') + '\n';
  out += titled("Verksamheten", '-') + '\n';
  out += "Allmänt om verksamheten\n";
  add_paragraph(out, std::format("Säte: {}", marker_of(check, "sate")), 2);
  add_paragraph(out, marker_of(check, "verksamhet"), 2);
  out += "\nVäsentliga händelser under räkenskapsåret\n";
  add_paragraph(out, marker_of(check, "handelser"), 2);
  return out;
}

std::string format_signatures(sie::Document const& document, std::vector<sie::FiscalYear> const& years,
                              Check const& check) {
  std::string out = titled("Underskrifter", '=') + '\n';
  out += std::format("Årsredovisning för {}, {}\n", document.company_name, document.orgnr);
  out += std::format("Avseende räkenskapsåret {} - {}\n\n", format_iso_date(years.front().start),
                     format_iso_date(years.front().end));
  add_paragraph(out, std::format("Ort och datum: {}", marker_of(check, "datering")));
  add_paragraph(out, std::format("Styrelseledamöter och verkställande direktör, med datum för underskrift: {}",
                                 marker_of(check, "underskrifter")));
  return out;
}

std::string format_check(Check const& check, std::string_view mapping) {
  std::string out = titled("K2 check", '=');
  add_paragraph(out, std::format("Checked against {}.", requirements_source));
  if (!mapping.empty()) {
    add_paragraph(out, std::format("Mapping: {}.", mapping));
  }
  out += '\n';
  if (check.missing.empty()) {
    out += "Complete: nothing is missing.\n";
  } else {
    add_paragraph(out, std::format("Not complete: {} {} missing, marked TBD [n] in the årsredovisning. Each breaks the "
                                   "rule given until it is provided.",
                                   check.missing.size(), check.missing.size() == 1 ? "item is" : "items are"));
  }
  // An item: its number, title and rule, then what was found (or when it applies), what to provide and where
  auto const add_item = [&out](CheckItem const& item, std::string_view detail_label) {
    auto const& requirement = *item.requirement;
    out += '\n';
    add_paragraph(out, std::format("[{}] {} - {}", item.number, requirement.title, requirement.rule), 0, 4);
    if (!item.detail.empty()) {
      add_paragraph(out, std::format("{}: {}.", detail_label, item.detail), 4);
    }
    add_paragraph(out, std::format("Provide: {}.", requirement.provide), 4);
    if (!requirement.heading.empty()) {
      add_paragraph(out, std::format("Template heading: ## {}", requirement.heading), 4);
    }
  };
  for (auto const& item : check.missing) {
    add_item(item, "Found");
  }
  if (!check.to_confirm.empty()) {
    out += '\n';
    add_paragraph(out, "Required only if it applies, which the SIE file does not tell. Confirm or provide:");
    for (auto const& item : check.to_confirm) {
      add_item(item, "When");
    }
  }
  return out;
}

std::string format_trace(Overview const& overview, Disposition const& disposition, EquityChanges const& equity,
                         Statement const& statement, Notes const& notes, sie::Document const& document,
                         std::vector<sie::FiscalYear> const& years) {
  std::string out = "Trace of the årsredovisning\n"
                    "Lines in kronor as on the årsredovisning; accounts and vouchers in kronor and öre as booked\n"
                    "(debit positive). Year 0 down to the vouchers; the SIE file has no vouchers for the year before.\n";
  auto const title = [&out](std::string_view text) {
    out += std::format("\n{}\n{}\n", text, std::string(width_of(text), '='));
  };
  auto const named = [&document](std::string_view account) {
    return std::format("{} {}", account, account_name(document, account));
  };
  // A voucher's transaction: the account with the transaction's own text, else the account's name
  auto const transaction_text = [&named](sie::Transaction const& transaction) {
    return transaction.text.empty() ? named(transaction.account) : std::format("{} {}", transaction.account, transaction.text);
  };
  auto const voucher_text = [](sie::Voucher const& voucher) {
    auto const id = voucher.number.empty() ? voucher.series : std::format("{} {}", voucher.series, voucher.number);
    return std::format("{} {} {}", id, format_iso_date(voucher.date), voucher.text);
  };
  auto const kronor = [](Amount amount) { return format_kronor(to_kronor(amount)) + " kr"; };

  std::vector<std::string> year_ends;
  for (auto const& year : years) {
    year_ends.push_back(format_iso_date(year.end));
  }
  auto const exact_columns = [](std::vector<Amount> const& amounts) {
    std::vector<std::string> columns;
    for (auto const amount : amounts) {
      columns.push_back(format_amount(amount));
    }
    return columns;
  };
  title("Flerårsöversikt");
  out += "Amounts as on the årsredovisning (not as booked), from the lines of the resultaträkning\n"
         "and balansräkning traced below.\n";
  add_row(out, "", 0, year_ends);
  out += '\n';
  add_row(out, "Nettoomsättning (tkr): Nettoomsättning", 0, exact_columns(overview.net_sales));
  add_row(out, "Resultat efter finansiella poster (tkr): Resultat efter finansiella poster", 0,
          exact_columns(overview.result_after_financial_items));
  std::vector<std::string> solidity;
  for (auto const& value : overview.solidity) {
    solidity.push_back(value ? format_percent(*value) + " %" : "-");
  }
  add_row(out, "Soliditet (%) = (Summa eget kapital + 79,4 % of Summa obeskattade reserver) / Summa tillgångar", 0,
          solidity);
  add_row(out, "Summa eget kapital", 2, exact_columns(overview.equity));
  add_row(out, "Summa obeskattade reserver", 2, exact_columns(overview.untaxed_reserves));
  add_row(out, "Summa tillgångar", 2, exact_columns(overview.assets));

  title("Resultatdisposition");
  out += "Amounts as on the årsredovisning, from the posts under Fritt eget kapital of the balansräkning.\n\n";
  for (auto const& row : disposition.funds) {
    add_row(out, row.text, 0, {format_amount(row.amount)});
  }
  add_row(out, "Summa", 0, {format_amount(disposition.total)});
  for (auto const& text : disposition.proposal) {
    add_row(out, std::format("{}: TBD, the board's proposal is not in the SIE file (see the K2 check)", text), 0, {});
  }

  title("Förändringar i eget kapital");
  auto const is_equity = [&equity](std::string_view account) {
    return std::ranges::any_of(equity.column_accounts, [account](AccountSet const& set) { return set.contains(account); });
  };
  for (auto const& row : equity.rows) {
    out += '\n';
    add_row(out, row.text, 0, {kronor(row.amounts.back())});
    for (auto const& [account, amount] : row.accounts) {
      if (amount != 0) {
        add_row(out, named(account), 2, {format_amount(amount)});
      }
    }
    for (auto const* voucher : row.vouchers) {
      add_row(out, voucher_text(*voucher), 2, {});
      for (auto const& transaction : voucher->transactions) {
        if (transaction.kind != sie::Transaction::Kind::removed && is_equity(transaction.account)) {
          add_row(out, transaction_text(transaction), 4, {format_amount(transaction.amount)});
        }
      }
    }
  }

  std::vector<std::string> header;
  for (auto const& year : years) {
    header.push_back(format_iso_date(year.end));
  }
  bool in_balance_sheet = false;
  title("Resultaträkning");
  add_row(out, "", 0, header);
  for (auto const& traced : trace_statement(statement, document)) {
    if (!in_balance_sheet && !traced.accounts.empty() && traced.accounts.front().account.starts_with('1')) {
      in_balance_sheet = true;
      title("Balansräkning");
      add_row(out, "", 0, header);
    }
    out += '\n';
    std::vector<std::string> totals;
    for (auto const amount : traced.line->exact) {
      totals.push_back(kronor(amount));
    }
    add_row(out, traced.line->text, 0, totals);
    for (auto const& account : traced.accounts) {
      std::vector<std::string> columns;
      for (auto const amount : account.amounts) {
        columns.push_back(format_amount(amount));
      }
      add_row(out, named(account.account), 2, columns);
      if (account.opening) {
        add_row(out, std::format("Ingående balans {}", format_iso_date(years.front().start)), 4,
                {format_amount(*account.opening)});
      }
      for (auto const& posting : account.postings) {
        add_row(out, voucher_text(*posting.voucher), 4, {format_amount(posting.amount)});
      }
    }
  }
  if (!notes.notes.empty()) {
    title("Noter");
    add_row(out, "", 0, {format_iso_date(years.front().end)});
  }
  for (auto const& note : notes.notes) {
    out += '\n';
    add_row(out, note.title, 0, {});
    for (auto const& section : note.sections) {
      out += '\n';
      add_row(out, section.text, 0, {});
      for (auto const& row : section.rows) {
        add_row(out, row.text, 2, {kronor(row.amount)});
        for (auto const& [account, amount] : row.accounts) {
          if (amount != 0) {
            add_row(out, named(account), 4, {format_amount(amount)});
          }
        }
        for (auto const* voucher : row.vouchers) {
          add_row(out, voucher_text(*voucher), 4, {});
          for (auto const& transaction : voucher->transactions) {
            if (transaction.kind != sie::Transaction::Kind::removed && section.accounts.contains(transaction.account)) {
              add_row(out, transaction_text(transaction), 6, {format_amount(transaction.amount)});
            }
          }
        }
      }
    }
  }
  return out;
}

} // namespace cratchit::ar
