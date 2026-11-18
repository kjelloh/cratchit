#pragma once

#include "core/amount.hpp"
#include "core/date.hpp"
#include "core/sie/lexer.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace cratchit::sie {

// The dimension and object pairs of an object list, e.g. {1 "100" 6 "P1"}
using Objects = std::vector<std::pair<std::string, std::string>>;

// #RAR: a fiscal year. Index 0 is the year of the file, -1 the year before, and so on.
struct FiscalYear {
  int index = 0;
  Date start;
  Date end;
};

// #ADRESS
struct Address {
  std::string contact;
  std::string street;
  std::string postal;
  std::string phone;
};

// #KONTO with what #KTYP, #SRU and #ENHET say about the same account
struct Account {
  std::string number;
  std::string name;
  std::string type; // T, S, K or I; empty when not given (then BAS decides)
  std::string sru;
  std::string unit;
};

// #DIM, or #UNDERDIM with its super dimension
struct Dimension {
  std::string number;
  std::string name;
  std::string super; // empty for #DIM
};

// #OBJEKT
struct Object {
  std::string dimension;
  std::string number;
  std::string name;
};

// #IB, #UB or #RES: a balance of an account in a fiscal year
struct Balance {
  int year = 0;
  std::string account;
  Amount amount = 0;
  std::string quantity;
};

// #OIB or #OUB: a balance of an account for objects
struct ObjectBalance {
  int year = 0;
  std::string account;
  Objects objects;
  Amount amount = 0;
  std::string quantity;
};

// #PSALDO or #PBUDGET: a balance of an account in a period (YYYYMM)
struct PeriodBalance {
  int year = 0;
  std::string period;
  std::string account;
  Objects objects;
  Amount amount = 0;
  std::string quantity;
};

// #TRANS, #RTRANS or #BTRANS in a voucher
struct Transaction {
  enum class Kind {
    normal,  // #TRANS
    added,   // #RTRANS, added after the voucher was registered (its duplicate #TRANS is skipped)
    removed, // #BTRANS, removed (struck) after the voucher was registered; does not count
  };

  Kind kind = Kind::normal;
  std::string account;
  Objects objects;
  Amount amount = 0;
  std::optional<Date> date; // when not given, the voucher's date
  std::string text;
  std::string quantity;
  std::string signature;

  bool operator==(Transaction const&) const = default;
};

// #VER with its transactions
struct Voucher {
  std::string series;
  std::string number;
  Date date;
  std::string text;
  std::optional<Date> registered;
  std::string signature;
  std::vector<Transaction> transactions;
  std::size_t line = 0; // where it starts in the file

  bool operator==(Voucher const& other) const {
    return series == other.series && number == other.number && date == other.date && text == other.text &&
           registered == other.registered && signature == other.signature && transactions == other.transactions;
  }
};

// #KSUMMA: whether the file is checksummed, and the checksum given and computed
struct Checksum {
  bool announced = false;             // the opening #KSUMMA without a value
  std::optional<std::uint32_t> given; // the closing #KSUMMA's value
  std::uint32_t computed = 0;         // CRC-32 of the records in between
};

// What an SIE file says, true to the file, with no bookkeeping meaning added
struct Document {
  std::string flag;      // #FLAGGA
  std::string format;    // #FORMAT
  std::string sie_type;  // #SIETYP
  std::string program;   // #PROGRAM name
  std::string program_version;
  std::optional<Date> generated; // #GEN
  std::string generated_by;
  std::string company_id;   // #FNR
  std::string company_name; // #FNAMN
  std::string orgnr;        // #ORGNR
  Address address;          // #ADRESS
  std::string company_type; // #FTYP
  std::string sni_code;     // #BKOD
  std::vector<FiscalYear> years;
  std::optional<Date> covers_until; // #OMFATTN
  std::string chart_type;           // #KPTYP
  std::string currency;             // #VALUTA
  std::string tax_year;             // #TAXAR
  std::vector<std::string> notes;   // #PROSA
  std::vector<Account> accounts;
  std::vector<Dimension> dimensions;
  std::vector<Object> objects;
  std::vector<Balance> opening;            // #IB
  std::vector<Balance> closing;            // #UB
  std::vector<Balance> results;            // #RES
  std::vector<ObjectBalance> object_opening; // #OIB
  std::vector<ObjectBalance> object_closing; // #OUB
  std::vector<PeriodBalance> period_balances; // #PSALDO
  std::vector<PeriodBalance> period_budgets;  // #PBUDGET
  std::vector<Voucher> vouchers;
  std::map<std::string, std::size_t> unknown_labels; // ignored, as SIE requires, with their count
  Checksum checksum;

  // The fiscal year with this index, if the file has it
  std::optional<FiscalYear> year(int index) const;
};

// A document and what was wrong in the lines it was read from
struct Parsed {
  Document document;
  std::vector<std::string> errors; // each names its line
};

// Reads lexed lines into a document. Unknown labels are counted and ignored.
// Extra fields at the end of a record are ignored. Errors: a lexer error, a
// missing or invalid field (date, amount, year number, object list), a
// transaction outside a voucher, a voucher without its { } block, a known
// label inside a block that is not a transaction, and a checksum announced
// but never given.
Parsed parse_document(std::vector<Lexed> const& lines);

// CRC-32 as SIE uses it (polynomial EDB88320, start FFFFFFFF, inverted at the end),
// over bytes added one at a time
class Crc32 {
public:
  void add(std::uint8_t byte);
  std::uint32_t value() const;

private:
  std::uint32_t crc_ = 0xFFFFFFFF;
};

} // namespace cratchit::sie
