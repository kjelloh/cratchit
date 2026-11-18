#pragma once

#include "core/books/books.hpp"
#include "core/date.hpp"
#include "core/statement/statement.hpp"

#include <cstddef>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

// Account statements read into drafts: each entry of a statement file, for
// the BAS account the user says the statement is for, becomes a draft with a
// single posting on that account. The file is only read: cratchit keeps no
// copy (beslut 2026-11-17); each posting's source names the file, relative to
// the project folder, and the line.
namespace cratchit {

// The statement file read: its entries and how they were found
struct StatementFile {
  std::filesystem::path file;
  statement::Document document;
};

// Reads and parses a statement file (see statement::parse_statement). Error
// text names the file. Does not log.
std::expected<StatementFile, std::string> read_statement_file(std::filesystem::path const& file);

// A BAS account suggested for a statement, and why
struct AccountGuess {
  std::string account;
  std::size_t matches = 0; // entries with a posting on the account of the same date and amount
};

// The account (from take 3) with the most entries of the document matched by
// postings in the books' vouchers (not removed): the same date and amount,
// each posting matching one entry. None on a tie or without a match.
std::optional<AccountGuess> guess_statement_account(books::Books const& books, statement::Document const& document);

// What reading the statement into drafts of a fiscal year would do
struct StatementDraftPlan {
  std::string account;
  std::vector<statement::Entry> added;   // become drafts, oldest first
  std::vector<statement::Entry> booked;  // a posting on the account in a voucher of the year has its date and amount
  std::vector<statement::Entry> drafted; // a posting on the account in a draft of the year has its date and amount
  std::vector<statement::Entry> outside; // outside the fiscal year: not read
};

// The plan for the document's entries on account in year: each entry inside
// the year is booked when an unused voucher posting (not removed) on account
// has its date (the posting's, else the event's) and amount, else drafted when
// an unused draft posting has, else added. Each posting matches one entry, so
// two equal entries need two postings.
StatementDraftPlan plan_statement_drafts(books::FiscalYear const& year, statement::Document const& document,
                                         std::string account);

// The source of a posting from a statement entry: the file as the inventory
// stores it (see stored_path) and the line, e.g. "bank/nordea.csv:5"
std::string statement_source(std::filesystem::path const& project_folder, std::filesystem::path const& file,
                             std::size_t line);

// Adds a draft for each entry of plan.added to the fiscal year starting on
// year_start, noted today: no heading and no amount, the date of the entry,
// and one posting on plan.account with the entry's amount (into the account
// is debit), date and text, and the entry's source. Writes the books and
// records the file in the year's inventory (record_statement_use). Gives the
// number of drafts added. Error text when the books cannot be read or
// written, or the year is not in them. Does not log.
std::expected<std::size_t, std::string> add_statement_drafts(std::filesystem::path const& project_folder,
                                                             std::filesystem::path const& file,
                                                             StatementDraftPlan const& plan, Date year_start,
                                                             Date today);

} // namespace cratchit
