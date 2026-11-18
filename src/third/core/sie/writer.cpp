#include "core/sie/writer.hpp"

#include "charset/cp437.hpp"
#include "charset/utf8.hpp"

#include <algorithm>
#include <chrono>
#include <format>
#include <set>

namespace cratchit::sie {

namespace {

std::string sie_quoted(std::string_view text) {
  std::string result = "\"";
  for (auto const c : text) {
    if (c == '"') {
      result += '\\';
    }
    result += c;
  }
  return result + '"';
}

// A field as it is, quoted when it would not lex as one field
std::string token(std::string_view text) {
  auto const plain = !text.empty() && std::ranges::none_of(text, [](char c) {
    return c == ' ' || c == '\t' || c == '"' || c == '{' || c == '}';
  });
  return plain ? std::string(text) : sie_quoted(text);
}

std::string date_text(Date date) {
  return std::format("{:04}{:02}{:02}", static_cast<int>(date.year()), static_cast<unsigned>(date.month()),
                     static_cast<unsigned>(date.day()));
}

// With a point and no trailing zeros, as Fortnox writes them
std::string amount_text(Amount amount) {
  auto text = format_amount(amount);
  if (auto const point = text.find('.'); point != std::string::npos) {
    while (text.ends_with('0')) {
      text.pop_back();
    }
    if (text.ends_with('.')) {
      text.pop_back();
    }
  }
  return text;
}

std::string objects_text(Objects const& objects) {
  std::string text = "{";
  for (auto const& [dimension, object] : objects) {
    if (text.size() > 1) {
      text += ' ';
    }
    text += std::format("{} {}", token(dimension), sie_quoted(object));
  }
  return text + "}";
}

class Writer {
public:
  void line(std::string_view text) {
    text_ += text;
    text_ += "\r\n";
  }
  void blank() { line(""); }
  std::string take() { return std::move(text_); }

private:
  std::string text_;
};

void write_identification(Document const& document, Writer& out) {
  out.line(std::format("#FLAGGA {}", token(document.flag.empty() ? "0" : document.flag)));
  if (!document.format.empty()) {
    out.line(std::format("#FORMAT {}", token(document.format)));
  }
  out.line(std::format("#SIETYP {}", token(document.sie_type)));
  if (!document.program.empty()) {
    out.line(std::format("#PROGRAM {} {}", sie_quoted(document.program), token(document.program_version)));
  }
  if (document.generated) {
    out.line(std::format("#GEN {}{}", date_text(*document.generated),
                         document.generated_by.empty() ? "" : " " + sie_quoted(document.generated_by)));
  }
  if (!document.company_id.empty()) {
    out.line(std::format("#FNR {}", token(document.company_id)));
  }
  out.line(std::format("#FNAMN {}", sie_quoted(document.company_name)));
  auto const& address = document.address;
  if (!address.contact.empty() || !address.street.empty() || !address.postal.empty() || !address.phone.empty()) {
    out.line(std::format("#ADRESS {} {} {} {}", sie_quoted(address.contact), sie_quoted(address.street),
                         sie_quoted(address.postal), sie_quoted(address.phone)));
  }
  for (auto const& year : document.years) {
    out.line(std::format("#RAR {} {} {}", year.index, date_text(year.start), date_text(year.end)));
  }
  if (!document.orgnr.empty()) {
    out.line(std::format("#ORGNR {}", token(document.orgnr)));
  }
  if (!document.company_type.empty()) {
    out.line(std::format("#FTYP {}", token(document.company_type)));
  }
  if (!document.sni_code.empty()) {
    out.line(std::format("#BKOD {}", token(document.sni_code)));
  }
  if (document.covers_until) {
    out.line(std::format("#OMFATTN {}", date_text(*document.covers_until)));
  }
  if (!document.chart_type.empty()) {
    out.line(std::format("#KPTYP {}", token(document.chart_type)));
  }
  if (!document.currency.empty()) {
    out.line(std::format("#VALUTA {}", token(document.currency)));
  }
  if (!document.tax_year.empty()) {
    out.line(std::format("#TAXAR {}", token(document.tax_year)));
  }
  for (auto const& note : document.notes) {
    out.line(std::format("#PROSA {}", sie_quoted(note)));
  }
}

void write_accounts(Document const& document, Writer& out) {
  for (auto const& account : document.accounts) {
    out.line(std::format("#KONTO {} {}", token(account.number), sie_quoted(account.name)));
    if (!account.type.empty()) {
      out.line(std::format("#KTYP {} {}", token(account.number), token(account.type)));
    }
    if (!account.sru.empty()) {
      out.line(std::format("#SRU {} {}", token(account.number), token(account.sru)));
    }
    if (!account.unit.empty()) {
      out.line(std::format("#ENHET {} {}", token(account.number), sie_quoted(account.unit)));
    }
  }
}

void write_dimensions(Document const& document, Writer& out) {
  for (auto const& dimension : document.dimensions) {
    if (dimension.super.empty()) {
      out.line(std::format("#DIM {} {}", token(dimension.number), sie_quoted(dimension.name)));
    } else {
      out.line(std::format("#UNDERDIM {} {} {}", token(dimension.number), sie_quoted(dimension.name),
                           token(dimension.super)));
    }
  }
  for (auto const& object : document.objects) {
    out.line(std::format("#OBJEKT {} {} {}", token(object.dimension), sie_quoted(object.number), sie_quoted(object.name)));
  }
}

std::string balance_line(std::string_view label, Balance const& balance) {
  return std::format("#{} {} {} {}{}", label, balance.year, token(balance.account), amount_text(balance.amount),
                     balance.quantity.empty() ? "" : " " + token(balance.quantity));
}

// #IB then #UB per account, a year at a time in the order of the years; then #RES a year at a time
void write_balances(Document const& document, Writer& out) {
  for (auto const& year : document.years) {
    auto written = false;
    std::set<std::string> done;
    for (auto const& opening : document.opening) {
      if (opening.year != year.index) {
        continue;
      }
      out.line(balance_line("IB", opening));
      written = true;
      auto const closing = std::ranges::find_if(document.closing, [&](Balance const& b) {
        return b.year == year.index && b.account == opening.account;
      });
      if (closing != document.closing.end()) {
        out.line(balance_line("UB", *closing));
        done.insert(opening.account);
      }
    }
    for (auto const& closing : document.closing) {
      if (closing.year == year.index && !done.contains(closing.account)) {
        out.line(balance_line("UB", closing));
        written = true;
      }
    }
    for (auto const& [label, balances] : {std::pair{"OIB", &document.object_opening},
                                          std::pair{"OUB", &document.object_closing}}) {
      for (auto const& balance : *balances) {
        if (balance.year == year.index) {
          out.line(std::format("#{} {} {} {} {}{}", label, balance.year, token(balance.account),
                               objects_text(balance.objects), amount_text(balance.amount),
                               balance.quantity.empty() ? "" : " " + token(balance.quantity)));
          written = true;
        }
      }
    }
    if (written) {
      out.blank();
    }
  }
  for (auto const& year : document.years) {
    auto written = false;
    for (auto const& result : document.results) {
      if (result.year == year.index) {
        out.line(balance_line("RES", result));
        written = true;
      }
    }
    if (written) {
      out.blank();
    }
  }
  for (auto const& [label, balances] : {std::pair{"PSALDO", &document.period_balances},
                                        std::pair{"PBUDGET", &document.period_budgets}}) {
    for (auto const& balance : *balances) {
      out.line(std::format("#{} {} {} {} {} {}{}", label, balance.year, token(balance.period),
                           token(balance.account), objects_text(balance.objects), amount_text(balance.amount),
                           balance.quantity.empty() ? "" : " " + token(balance.quantity)));
    }
  }
}

// account {objects} amount date text [quantity [signature]]; date and text as "" when not given
std::string transaction_line(std::string_view label, Transaction const& t, bool with_signature) {
  auto line = std::format("#{} {} {} {} {} {}", label, token(t.account), objects_text(t.objects),
                          amount_text(t.amount), t.date ? date_text(*t.date) : "\"\"", sie_quoted(t.text));
  auto const signature = with_signature && !t.signature.empty();
  if (!t.quantity.empty() || signature) {
    line += " " + (t.quantity.empty() ? std::string("\"\"") : token(t.quantity));
  }
  if (signature) {
    line += " " + sie_quoted(t.signature);
  }
  return line;
}

void write_vouchers(Document const& document, Writer& out) {
  for (auto const& voucher : document.vouchers) {
    auto line = std::format("#VER {} {} {} {}", token(voucher.series), token(voucher.number),
                            date_text(voucher.date), sie_quoted(voucher.text));
    if (voucher.registered || !voucher.signature.empty()) {
      line += " " + (voucher.registered ? date_text(*voucher.registered) : std::string("\"\""));
    }
    if (!voucher.signature.empty()) {
      line += " " + sie_quoted(voucher.signature);
    }
    out.line(line);
    out.line("{");
    for (auto const& t : voucher.transactions) {
      switch (t.kind) {
      case Transaction::Kind::added:
        out.line(transaction_line("RTRANS", t, true));
        out.line(transaction_line("TRANS", t, false));
        break;
      case Transaction::Kind::removed:
        out.line(transaction_line("BTRANS", t, true));
        break;
      default:
        out.line(transaction_line("TRANS", t, true));
        break;
      }
    }
    out.line("}");
  }
}

} // namespace

std::string text_of(Document const& document) {
  Writer out;
  write_identification(document, out);
  out.blank();
  if (!document.accounts.empty()) {
    write_accounts(document, out);
    out.blank();
  }
  if (!document.dimensions.empty() || !document.objects.empty()) {
    write_dimensions(document, out);
    out.blank();
  }
  write_balances(document, out);
  write_vouchers(document, out);
  return out.take();
}

Encoded cp437_of(std::string_view text) {
  Encoded encoded;
  charset::utf8::Decoder from_utf8;
  charset::cp437::Encoder to_cp437;
  auto const put = [&](charset::unicode::CodePoint code_point) {
    if (auto const byte = to_cp437.push(code_point)) {
      encoded.bytes.push_back(*byte);
    } else {
      for (auto const c : std::format("<U+{:04X}>", static_cast<std::uint32_t>(code_point))) {
        encoded.bytes.push_back(static_cast<std::uint8_t>(c));
      }
      ++encoded.unmapped;
    }
  };
  for (auto const c : text) {
    if (auto const code_point = from_utf8.push(static_cast<charset::utf8::CodePoint>(c))) {
      put(*code_point);
    }
  }
  if (auto const code_point = from_utf8.finish()) {
    put(*code_point);
  }
  return encoded;
}

} // namespace cratchit::sie
