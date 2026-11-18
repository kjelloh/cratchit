#include "core/ink2/output.hpp"

#include <algorithm>
#include <format>

namespace cratchit::ink2 {

namespace {

constexpr std::size_t number_width = 6;
constexpr std::size_t text_width = 58;
constexpr std::size_t amount_width = 12;

// The number of code points of UTF-8 text, as wide as it shows in a terminal for the texts of the form
std::size_t width_of(std::string_view text) {
  return static_cast<std::size_t>(
      std::ranges::count_if(text, [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; }));
}

std::string padded(std::string_view text, std::size_t width) {
  auto const shown = width_of(text);
  return std::string(text) + std::string(shown < width ? width - shown : 0, ' ');
}

// Text in lines of at most width, broken at spaces (a longer word gets a line of its own)
std::vector<std::string> wrapped(std::string_view text, std::size_t width) {
  std::vector<std::string> lines;
  std::string line;
  std::size_t position = 0;
  while (position < text.size()) {
    auto const start = text.find_first_not_of(' ', position);
    if (start == std::string_view::npos) {
      break;
    }
    auto end = text.find(' ', start);
    if (end == std::string_view::npos) {
      end = text.size();
    }
    auto const word = text.substr(start, end - start);
    if (!line.empty() && width_of(line) + 1 + width_of(word) > width) {
      lines.push_back(std::move(line));
      line.clear();
    }
    line += (line.empty() ? "" : " ") + std::string(word);
    position = end;
  }
  if (!line.empty() || lines.empty()) {
    lines.push_back(std::move(line));
  }
  return lines;
}

std::string amount_of(FieldAmount const& field) {
  return field.used ? format_kronor(field.kronor) : "";
}

void append_sum(std::string& out, std::string_view text, Kronor kronor) {
  out += std::format("  {}{:>{}}\n", padded(text, number_width + text_width + 10), format_kronor(kronor), amount_width);
}

// A row: number, text, SRU code with its marker, amount, and what to show after it
std::string row_of(std::string_view number, std::string_view text, std::string_view sru, std::string_view sign,
                   std::string_view amount, std::string_view marker) {
  auto row = std::format("  {}{}", padded(number, number_width), padded(text, text_width));
  if (!sru.empty()) {
    row += std::format("  {:<4}{}  {:>{}}", sru, sign, amount, amount_width);
  }
  if (!marker.empty()) {
    row += "  " + std::string(marker);
  }
  while (row.ends_with(' ')) {
    row.pop_back();
  }
  return row + '\n';
}

std::string marker_of(Markers const& markers, std::string const& sru) {
  auto const found = markers.find(sru);
  return found == markers.end() ? "" : found->second;
}

} // namespace

std::string format_kronor(Kronor kronor) {
  auto const digits = std::to_string(kronor < 0 ? -kronor : kronor);
  std::string out;
  for (std::size_t index = 0; index < digits.size(); ++index) {
    if (index != 0 && (digits.size() - index) % 3 == 0) {
      out += ' ';
    }
    out += digits[index];
  }
  return (kronor < 0 ? "-" : "") + out;
}

std::string format_head(Header const& header) {
  std::string out;
  out += std::format("Inkomstdeklaration 2: {}, {}\n", header.company, header.orgnr);
  out += std::format("Räkenskapsår {} - {}   7011 {}   7012 {}\n", format_iso_date(header.start),
                     format_iso_date(header.end), format_compact_date(header.start), format_compact_date(header.end));
  if (!header.status.empty()) {
    out += std::format("Status: {}\n", header.status);
  }
  return out;
}

std::string format_form(std::vector<Line> const& lines, Form const& form, Markers const& markers) {
  std::string out = "INK2R Räkenskapsschema\n";
  bool after_row = true;
  for (auto const& line : lines) {
    if (line.form != "INK2R") {
      continue;
    }
    if (line.kind == Line::Kind::heading) {
      out += std::format("{}{}\n", after_row ? "\n" : "", line.text);
      after_row = false;
      continue;
    }
    after_row = true;
    auto const texts = wrapped(line.text, text_width);
    auto const count = std::max(texts.size(), line.fields.size());
    bool const paired = line.fields.size() > 1;
    for (std::size_t index = 0; index < count; ++index) {
      auto const number = index == 0 ? line.ink2 : std::string();
      auto const text = index < texts.size() ? texts[index] : std::string();
      if (index < line.fields.size()) {
        auto const& sru = line.fields[index].sru;
        auto const sign = !paired ? "  " : index == 0 ? " +" : " -";
        out += row_of(number, text, sru, sign, amount_of(form.fields.at(sru)), marker_of(markers, sru));
      } else {
        out += row_of(number, text, "", "", "", "");
      }
    }
  }
  auto const sums = sums_of(form);
  out += "\nSummor av fälten, i hela kronor\n";
  append_sum(out, "Tillgångar (72xx)", sums.assets);
  append_sum(out, "Eget kapital och skulder (73xx)", sums.equity_and_debt);
  append_sum(out, "Resultaträkningens fält (74xx - 75xx, utom 3.26-3.27)", sums.result);
  append_sum(out, "Årets resultat (3.26 - 3.27)", sums.year_result);
  return out;
}

std::string format_skv_form(std::string_view title, std::vector<SkvField> const& fields,
                            std::map<std::string, Kronor> const& amounts, std::set<std::string> const& marks,
                            Markers const& markers) {
  std::string out = std::format("{}\n", title);
  for (auto const& field : fields) {
    std::string value;
    if (field.type == SkvField::Type::mark) {
      value = marks.contains(field.sru) ? "X" : "";
    } else if (auto const amount = amounts.find(field.sru); amount != amounts.end()) {
      value = format_kronor(amount->second);
    }
    auto const sign = field.sign == '+' ? " +" : field.sign == '-' ? " -" : "  ";
    auto const texts = wrapped(field.text, text_width);
    for (std::size_t index = 0; index < texts.size(); ++index) {
      out += index == 0 ? row_of(field.ink2, texts[index], field.sru, sign, value, marker_of(markers, field.sru))
                        : row_of("", texts[index], "", "", "", "");
    }
  }
  return out;
}

std::string format_return(Header const& header, std::vector<Line> const& lines, std::vector<SkvField> const& skv,
                          Form const& form, TaxReturn const& tax, Markers const& markers) {
  return std::format("{}\n{}\n{}\n{}", format_head(header),
                     format_skv_form("INK2 Inkomstdeklaration 2, sidan 1", fields_of(skv, "INK2"), tax.ink2, {},
                                     markers),
                     format_form(lines, form, markers),
                     format_skv_form("INK2S Skattemässiga justeringar", fields_of(skv, "INK2S"), tax.ink2s,
                                     tax.ink2s_marks, markers));
}

} // namespace cratchit::ink2
