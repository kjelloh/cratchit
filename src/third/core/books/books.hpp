#pragma once

#include "core/amount.hpp"
#include "core/bas.hpp"
#include "core/date.hpp"

#include <cstddef>
#include <expected>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// cratchit's own books of a project: the chart of accounts, the fiscal years
// and their vouchers. Independent of where the data came from (e.g. SIE).
namespace cratchit::books {

struct Account {
  std::string number;
  std::string name;
  std::string type; // T, S, K or I when given; else the BAS class decides (see account_kind)
  std::string sru;
  std::string unit;

  bool operator==(Account const&) const = default;
};

struct Dimension {
  std::string number;
  std::string name;
  std::string super; // the super dimension of a sub dimension, else empty

  bool operator==(Dimension const&) const = default;
};

struct Object {
  std::string dimension;
  std::string number;
  std::string name;

  bool operator==(Object const&) const = default;
};

// Dimension and object pairs
using Objects = std::vector<std::pair<std::string, std::string>>;

// One amount posted to one account (debit positive, credit negative)
struct Posting {
  enum class Kind {
    normal,
    added,   // added after the voucher was registered
    removed, // removed (struck) after the voucher was registered; kept for the history, does not count
  };

  Kind kind = Kind::normal;
  std::string account;
  Objects objects{};
  Amount amount = 0;
  std::optional<Date> date{}; // when not given, the event's date
  std::string text{};
  std::string quantity{};
  std::string signature{};
  std::string source{}; // the statement entry it came from (stashed file:line); kept on drafts only

  bool operator==(Posting const&) const = default;
};

// The postings of a financial event (removed ones are kept but do not count)
using Postings = std::vector<Posting>;

// The sum of the debit postings, and of the credit postings (without sign), not counting removed ones
Amount debit(Postings const& postings);
Amount credit(Postings const& postings);

// The sum of the postings, not counting removed ones; zero when they balance
Amount sum(Postings const& postings);

// How a financial event enters the books: what happened, how much, and when.
// In a voucher the amount is without sign: what the debit and the credit
// postings each add up to.
struct Event {
  std::string heading;
  Amount amount = 0;
  Date date;

  bool operator==(Event const&) const = default;
};

// A voucher's id as the user sees and types it: series and number as one word, e.g. "A12", "D1"
std::string voucher_id(std::string_view series, std::string_view number);

// The series and number of an id typed as one word: the letters first, then
// digits only, e.g. "A12" gives {"A", "12"}. Nothing for anything else (a
// space, no series, no number, or letters after the digits).
std::optional<std::pair<std::string, std::string>> parse_voucher_id(std::string_view text);

// A financial event in the books: the event and its postings, numbered in a
// series. Once in the books it is never changed.
struct Voucher {
  std::string series;
  std::string number;
  Event event;
  std::optional<Date> registered;
  std::string signature;
  Postings postings;

  bool operator==(Voucher const&) const = default;
};

// A financial event on its way into the books: anything in it may change
// until it is complete and booked as a voucher. It may hold just the event
// (e.g. an invoice or a receipt noted), just postings (e.g. from statement
// entries), or both. It stores no kind; see stage.
struct Draft {
  // The amount with its sign (beslut 2026-11-17): minus for an event that goes the other
  // way than usual, e.g. a credit note or a refund, so that a template can be turned to
  // fit it; the voucher it is booked as has the amount without sign. Heading and amount may be empty.
  Event event;
  Date noted;  // the day it was noted
  Postings postings{}; // each with its date and text
  // The series to book it in, e.g. from the template used (beslut 2026-11-18); the user
  // may change it, and Bokför starts with it. Empty when none.
  std::string series{};

  bool operator==(Draft const&) const = default;
};

// What a draft is made of (beslut 2026-11-17). Its parts are its event (when
// it has a heading or an amount) and each posting (not removed).
enum class DraftParts {
  lone, // one part: just the event, or just one posting (an account event); can be forgotten
  same, // more than one part, all with the same amount without sign (the event's
        // amount, when it has one, and each posting's); can be split (Inte samma)
  more, // more than one part with different amounts: more than the same event; neither
};

DraftParts parts_of(Draft const& draft);

// How far a draft has come, worked out from what it holds
enum class DraftStage {
  event,         // no postings: just the event (also when its heading is empty)
  account_event, // just one posting, no heading and no amount: e.g. an entry of a statement
  same,          // more than one part, all of the same amount (parts_of same), but not ready
  unbalanced,    // more than one part of different amounts, not ready
  ready,      // can be booked: a heading, an amount, postings each on an account,
              // that balance and whose debit adds up to the amount (without its sign)
};

// The stage of draft (removed postings do not count)
DraftStage stage(Draft const& draft);

struct FiscalYear {
  Date start;
  Date end;
  // The opening balances the source reported. Used only when the year before
  // is not in the books; else the year before's closing balances are carried.
  std::map<std::string, Amount> reported_opening{};
  std::vector<Voucher> vouchers{};
  bool closed = false; // closed for new vouchers (open by default)
  // Drafts not yet in the books, oldest date first, ties in the order noted.
  // Add them with place_draft only, so the order holds.
  std::vector<Draft> drafts{};

  // True when date is within the year
  bool holds(Date date) const;
  // Places draft among the drafts by its date, after those of the same date; its index
  std::size_t place_draft(Draft draft);
};

struct Address {
  std::string contact;
  std::string street;
  std::string postal;
  std::string phone;

  bool operator==(Address const&) const = default;
};

// A voucher series and its name, e.g. D "Leverantörsfakturor". SIE gives no
// names to series, so the books name their own (books/series.csv).
struct Series {
  std::string series;
  std::string name; // empty when not named
  bool operator==(Series const&) const = default;
};

// Fortnox's voucher series A to N with Fortnox's names: the series of books
// that have no list of their own yet
std::vector<Series> const& default_series();

struct Books {
  std::string chart_type; // e.g. EUBAS97
  std::string company_id; // the source program's id of the company
  Address address;
  std::vector<Account> accounts;     // by number
  std::vector<Dimension> dimensions; // by number
  std::vector<Object> objects;
  std::vector<FiscalYear> years; // by start date
  // The series and their names, by series: books/series.csv, else
  // default_series(); each series a voucher uses is in it (unnamed when new)
  std::vector<Series> series;

  FiscalYear const* year_starting(Date start) const;
  Account const* account(std::string_view number) const;
  // The name of the series, empty when it has none
  std::string series_name(std::string_view series) const;
};

// The fiscal year starting on start, added to books when it comes after the
// last year in them (beslut 2026-11-18: the books run on over the years): each
// year after the last is twelve months, from the day after the year before
// ends, and the years between are added too, so that each opens with the
// closing balances of the year before (see balances). nullptr when start is
// not the start of such a year, or comes before the last year without being
// in the books (older years come only from SIE files), or books has no year.
FiscalYear* continued_year(Books& books, Date start);

// The kind of an account in the books (by its type, else its BAS class)
AccountKind account_kind(Books const& books, std::string_view number);

// The balances of one fiscal year, per account
struct YearBalances {
  std::map<std::string, Amount> opening; // balance accounts only
  std::map<std::string, Amount> change;  // the year's postings (not removed ones)
  std::map<std::string, Amount> closing; // opening + change
  bool carried = false; // opening carried from the year before's closing, not as reported
};

// The balances of every year, in the order of books.years. A year directly
// after another in the books opens with that year's closing balances of the
// balance accounts; any other year opens with its reported opening balances.
// Result accounts open at zero.
std::vector<YearBalances> balances(Books const& books);

// A file written by write_books
struct Written {
  std::filesystem::path file; // relative to the project folder, e.g. .cratchit/books/accounts.csv
  bool created = false;       // else updated (or removed)
  bool removed = false;       // removed, as it has nothing left to hold
};

// The folder holding the books: <project folder>/.cratchit/books
std::filesystem::path books_folder(std::filesystem::path const& project_folder);

// Reads the books of a project. No books folder gives empty books. Missing
// accounts.csv, dimensions.csv or objects.csv are empty, and so is a year
// without a folder of its own (as in books made from details); a file missing
// in a year's folder is an error.
// Error text names the file that is missing or malformed.
std::expected<Books, std::string> read_books(std::filesystem::path const& project_folder);

// Writes the books of a project, creating folders as needed. Only files whose
// content changes are written; those are returned, in the order written. A
// year's events.csv exists only while the year has drafts, and its
// event_postings.csv only while they have postings. A year's
// postings are in postings.csv; transactions.csv, its name before, is read
// when postings.csv is missing and removed on writing.
std::expected<std::vector<Written>, std::string> write_books(std::filesystem::path const& project_folder,
                                                             Books const& books);

} // namespace cratchit::books
