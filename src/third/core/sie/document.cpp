#include "core/sie/document.hpp"

#include "charset/cp437.hpp"
#include "charset/utf8.hpp"

#include <algorithm>
#include <limits>
#include <charconv>
#include <format>
#include <functional>
#include <string_view>

namespace cratchit::sie {

namespace {

constexpr std::string_view known_labels[] = {
    "ADRESS", "BKOD",   "BTRANS", "DIM",     "ENHET",   "FLAGGA", "FNAMN",  "FNR",      "FORMAT",
    "FTYP",   "GEN",    "IB",     "KONTO",   "KPTYP",   "KSUMMA", "KTYP",   "OBJEKT",   "OIB",
    "OMFATTN", "ORGNR", "OUB",    "PBUDGET", "PROGRAM", "PROSA",  "PSALDO", "RAR",      "RES",
    "RTRANS", "SIETYP", "SRU",    "TAXAR",   "TRANS",   "UB",     "UNDERDIM", "VALUTA", "VER"};

bool is_known(std::string_view label) {
  return std::ranges::find(known_labels, label) != std::ranges::end(known_labels);
}

bool is_transaction(std::string_view label) {
  return label == "TRANS" || label == "RTRANS" || label == "BTRANS";
}

// The fields of one record line, read with checks that collect errors
class Fields {
public:
  Fields(Line const& line, std::vector<std::string>& errors) : line_(line), errors_(errors) {}

  bool ok() const { return ok_; }

  // Text of field i; empty when missing
  std::string text(std::size_t i) const {
    if (i >= line_.fields.size()) {
      return {};
    }
    if (auto const* text = std::get_if<std::string>(&line_.fields[i])) {
      return *text;
    }
    fail(std::format("field {} is an object list, not text", i + 1));
    return {};
  }

  // Text of field i, which must be given and not empty
  std::string required(std::size_t i, std::string_view what) const {
    auto value = text(i);
    if (value.empty() && ok_) {
      fail(std::format("no {}", what));
    }
    return value;
  }

  Date date(std::size_t i, std::string_view what) const {
    auto const value = required(i, what);
    if (!ok_) {
      return {};
    }
    auto const date = parse_compact_date(value);
    if (!date) {
      fail(std::format("{} '{}' is not a date YYYYMMDD", what, value));
      return {};
    }
    return *date;
  }

  // A date that may be left out (missing or empty)
  std::optional<Date> optional_date(std::size_t i, std::string_view what) const {
    auto const value = text(i);
    if (value.empty()) {
      return std::nullopt;
    }
    auto const date = parse_compact_date(value);
    if (!date) {
      fail(std::format("{} '{}' is not a date YYYYMMDD", what, value));
    }
    return date;
  }

  Amount amount(std::size_t i, std::string_view what) const {
    auto const value = required(i, what);
    if (!ok_) {
      return 0;
    }
    auto const amount = parse_amount(value);
    if (!amount) {
      fail(std::format("{} '{}' is not an amount", what, value));
      return 0;
    }
    return *amount;
  }

  int year(std::size_t i) const {
    auto const value = required(i, "year number");
    if (!ok_) {
      return 0;
    }
    int year = 0;
    auto const [end, ec] = std::from_chars(value.data(), value.data() + value.size(), year);
    if (ec != std::errc{} || end != value.data() + value.size()) {
      fail(std::format("year number '{}' is not a number", value));
    }
    return year;
  }

  Objects objects(std::size_t i) const {
    if (i >= line_.fields.size()) {
      fail("no object list");
      return {};
    }
    auto const* list = std::get_if<ObjectList>(&line_.fields[i]);
    if (!list) {
      fail(std::format("field {} is not an object list {{...}}", i + 1));
      return {};
    }
    if (list->items.size() % 2 != 0) {
      fail("an object list must hold pairs of dimension and object");
      return {};
    }
    Objects objects;
    for (std::size_t k = 0; k < list->items.size(); k += 2) {
      objects.emplace_back(list->items[k], list->items[k + 1]);
    }
    return objects;
  }

private:
  void fail(std::string what) const {
    if (ok_) {
      errors_.push_back(std::format("line {}: #{}: {}", line_.number, line_.label, what));
      ok_ = false;
    }
  }

  Line const& line_;
  std::vector<std::string>& errors_;
  mutable bool ok_ = true;
};

Account& account_in(Document& document, std::string const& number) {
  auto it = std::ranges::find(document.accounts, number, &Account::number);
  if (it == document.accounts.end()) {
    document.accounts.push_back({number, {}, {}, {}, {}});
    return document.accounts.back();
  }
  return *it;
}

// Adds the text of a record to the checksum in CP437: the label with its #
// and every field, without separators, quotes or braces. A quote inside a
// field counts, its escaping backslash does not, as the SIE 4B example (10.15)
// underlines: #KONTO 1915 "Kassa \"special\"" sums `#KONTO1915Kassa "special"`.
void add_to_checksum(Crc32& crc, Line const& line) {
  auto const add_text = [&crc](std::string_view text) {
    charset::utf8::Decoder to_unicode;
    charset::cp437::Encoder to_cp437;
    for (char const c : text) {
      if (auto const code_point = to_unicode.push(static_cast<charset::utf8::CodePoint>(c))) {
        if (auto const byte = to_cp437.push(*code_point)) {
          crc.add(*byte);
        }
      }
    }
  };
  crc.add('#');
  add_text(line.label);
  for (auto const& field : line.fields) {
    if (auto const* text = std::get_if<std::string>(&field)) {
      add_text(*text);
    } else {
      for (auto const& item : std::get<ObjectList>(field).items) {
        add_text(item);
      }
    }
  }
}

std::optional<std::uint32_t> parse_checksum(std::string_view text) {
  std::int64_t value = 0;
  auto const [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
  if (ec != std::errc{} || end != text.data() + text.size() || value < std::numeric_limits<std::int32_t>::min() ||
      value > std::numeric_limits<std::uint32_t>::max()) {
    return std::nullopt;
  }
  return static_cast<std::uint32_t>(value);
}

class Parser {
public:
  Parsed run(std::vector<Lexed> const& lines) {
    for (auto const& lexed : lines) {
      if (!lexed) {
        result_.errors.push_back(lexed.error());
        continue;
      }
      on_line(*lexed);
    }
    if (voucher_) {
      error(voucher_->line, "#VER: its { } block is not closed");
      end_voucher();
    }
    if (checksumming_) {
      result_.errors.push_back("#KSUMMA: the file announces a checksum but does not end with one");
    }
    result_.document.checksum.computed = crc_.value();
    return std::move(result_);
  }

private:
  void error(std::size_t line, std::string what) { result_.errors.push_back(std::format("line {}: {}", line, what)); }

  void on_line(Line const& line) {
    auto& document = result_.document;
    if (line.kind == Line::Kind::block_begin) {
      if (!voucher_ || in_block_) {
        error(line.number, in_block_ ? "{ inside a voucher's block" : "{ without a #VER before it");
        return;
      }
      in_block_ = true;
      return;
    }
    if (line.kind == Line::Kind::block_end) {
      if (!in_block_) {
        error(line.number, "} without a { before it");
        return;
      }
      end_voucher();
      return;
    }
    if (line.label == "KSUMMA") {
      on_checksum(line);
      return;
    }
    if (checksumming_) {
      add_to_checksum(crc_, line);
    }
    if (!is_known(line.label)) {
      ++document.unknown_labels[line.label];
      return;
    }
    if (voucher_ && !in_block_) {
      error(voucher_->line, "#VER: no { } block with its transactions");
      end_voucher();
    }
    if (in_block_ && !is_transaction(line.label)) {
      error(line.number, std::format("#{} inside a voucher's block", line.label));
      return;
    }
    Fields const f(line, result_.errors);
    auto const& label = line.label;
    if (label == "FLAGGA") {
      document.flag = f.required(0, "flag");
    } else if (label == "FORMAT") {
      document.format = f.required(0, "format");
    } else if (label == "SIETYP") {
      document.sie_type = f.required(0, "type number");
    } else if (label == "PROGRAM") {
      document.program = f.required(0, "program name");
      document.program_version = f.text(1);
    } else if (label == "GEN") {
      auto const date = f.date(0, "date");
      if (f.ok()) {
        document.generated = date;
      }
      document.generated_by = f.text(1);
    } else if (label == "FNR") {
      document.company_id = f.required(0, "company id");
    } else if (label == "FNAMN") {
      document.company_name = f.required(0, "company name");
    } else if (label == "ORGNR") {
      document.orgnr = f.required(0, "organisation number");
    } else if (label == "ADRESS") {
      document.address = {f.text(0), f.text(1), f.text(2), f.text(3)};
    } else if (label == "FTYP") {
      document.company_type = f.text(0);
    } else if (label == "BKOD") {
      document.sni_code = f.text(0);
    } else if (label == "RAR") {
      FiscalYear year{f.year(0), f.date(1, "start date"), f.date(2, "end date")};
      if (f.ok()) {
        document.years.push_back(year);
      }
    } else if (label == "OMFATTN") {
      auto const date = f.date(0, "date");
      if (f.ok()) {
        document.covers_until = date;
      }
    } else if (label == "KPTYP") {
      document.chart_type = f.text(0);
    } else if (label == "VALUTA") {
      document.currency = f.text(0);
    } else if (label == "TAXAR") {
      document.tax_year = f.text(0);
    } else if (label == "PROSA") {
      document.notes.push_back(f.text(0));
    } else if (label == "KONTO") {
      auto const number = f.required(0, "account number");
      if (f.ok()) {
        account_in(document, number).name = f.text(1);
      }
    } else if (label == "KTYP" || label == "SRU" || label == "ENHET") {
      auto const number = f.required(0, "account number");
      auto const value = f.required(1, label == "KTYP" ? "account type" : label == "SRU" ? "SRU code" : "unit");
      if (f.ok()) {
        auto& account = account_in(document, number);
        (label == "KTYP" ? account.type : label == "SRU" ? account.sru : account.unit) = value;
      }
    } else if (label == "DIM") {
      Dimension dimension{f.required(0, "dimension number"), f.text(1), {}};
      if (f.ok()) {
        document.dimensions.push_back(std::move(dimension));
      }
    } else if (label == "UNDERDIM") {
      Dimension dimension{f.required(0, "dimension number"), f.text(1), f.required(2, "super dimension")};
      if (f.ok()) {
        document.dimensions.push_back(std::move(dimension));
      }
    } else if (label == "OBJEKT") {
      Object object{f.required(0, "dimension number"), f.required(1, "object number"), f.text(2)};
      if (f.ok()) {
        document.objects.push_back(std::move(object));
      }
    } else if (label == "IB" || label == "UB" || label == "RES") {
      Balance balance{f.year(0), f.required(1, "account number"), f.amount(2, "balance"), f.text(3)};
      if (f.ok()) {
        (label == "IB" ? document.opening : label == "UB" ? document.closing : document.results).push_back(balance);
      }
    } else if (label == "OIB" || label == "OUB") {
      ObjectBalance balance{f.year(0), f.required(1, "account number"), f.objects(2), f.amount(3, "balance"),
                            f.text(4)};
      if (f.ok()) {
        (label == "OIB" ? document.object_opening : document.object_closing).push_back(std::move(balance));
      }
    } else if (label == "PSALDO" || label == "PBUDGET") {
      PeriodBalance balance{f.year(0),           f.required(1, "period"), f.required(2, "account number"),
                            f.objects(3),        f.amount(4, "balance"),  f.text(5)};
      if (f.ok()) {
        (label == "PSALDO" ? document.period_balances : document.period_budgets).push_back(std::move(balance));
      }
    } else if (label == "VER") {
      Voucher voucher{f.text(0), f.text(1), f.date(2, "voucher date"), f.text(3), {}, {}, {}, line.number};
      voucher.registered = f.optional_date(4, "registration date");
      voucher.signature = f.text(5);
      // Kept even when a field is wrong, so its block is read as its block
      voucher_ = std::move(voucher);
    } else if (is_transaction(label)) {
      on_transaction(line, f);
    }
  }

  void on_transaction(Line const& line, Fields const& f) {
    if (!in_block_) {
      error(line.number, std::format("#{} outside a voucher's {{ }} block", line.label));
      return;
    }
    Transaction transaction;
    transaction.kind = line.label == "RTRANS"   ? Transaction::Kind::added
                       : line.label == "BTRANS" ? Transaction::Kind::removed
                                                : Transaction::Kind::normal;
    transaction.account = f.required(0, "account number");
    transaction.objects = f.objects(1);
    transaction.amount = f.amount(2, "amount");
    transaction.date = f.optional_date(3, "transaction date");
    transaction.text = f.text(4);
    transaction.quantity = f.text(5);
    transaction.signature = f.text(6);
    if (!f.ok()) {
      added_.reset();
      return;
    }
    // An #RTRANS is followed by a like #TRANS for programs that do not know #RTRANS; skip it
    if (transaction.kind == Transaction::Kind::normal && added_ && added_->account == transaction.account &&
        added_->objects == transaction.objects && added_->amount == transaction.amount) {
      added_.reset();
      return;
    }
    added_.reset();
    if (transaction.kind == Transaction::Kind::added) {
      added_ = transaction;
    }
    voucher_->transactions.push_back(std::move(transaction));
  }

  void on_checksum(Line const& line) {
    auto& checksum = result_.document.checksum;
    if (line.fields.empty()) {
      if (checksum.announced) {
        error(line.number, "#KSUMMA: announced twice");
        return;
      }
      checksum.announced = true;
      checksumming_ = true;
      return;
    }
    Fields const f(line, result_.errors);
    auto const text = f.text(0);
    if (!checksum.announced) {
      error(line.number, "#KSUMMA: a checksum without the opening #KSUMMA");
      return;
    }
    checksumming_ = false;
    checksum.given = parse_checksum(text);
    if (!checksum.given) {
      error(line.number, std::format("#KSUMMA: '{}' is not a checksum", text));
    }
  }

  void end_voucher() {
    result_.document.vouchers.push_back(std::move(*voucher_));
    voucher_.reset();
    in_block_ = false;
    added_.reset();
  }

  Parsed result_;
  std::optional<Voucher> voucher_; // the voucher being read
  bool in_block_ = false;
  std::optional<Transaction> added_; // the #RTRANS just read
  bool checksumming_ = false;
  Crc32 crc_;
};

} // namespace

std::optional<FiscalYear> Document::year(int index) const {
  auto const it = std::ranges::find(years, index, &FiscalYear::index);
  if (it == years.end()) {
    return std::nullopt;
  }
  return *it;
}

Parsed parse_document(std::vector<Lexed> const& lines) {
  return Parser{}.run(lines);
}

void Crc32::add(std::uint8_t byte) {
  crc_ ^= byte;
  for (int bit = 0; bit < 8; ++bit) {
    crc_ = (crc_ & 1) != 0 ? (crc_ >> 1) ^ 0xEDB88320u : crc_ >> 1;
  }
}

std::uint32_t Crc32::value() const {
  return crc_ ^ 0xFFFFFFFFu;
}

} // namespace cratchit::sie
