#include "core/statement_drafts.hpp"

#include "core/inventory.hpp"

#include <algorithm>
#include <format>
#include <fstream>
#include <iterator>
#include <map>
#include <utility>

namespace cratchit {

namespace fs = std::filesystem;

namespace {

// Postings by account, date and amount, each a count of how many are left to match
using Key = std::pair<Date, Amount>;
using Pool = std::map<Key, std::size_t>;

// Takes one from the pool when it has the key
bool take(Pool& pool, Key const& key) {
  auto const found = pool.find(key);
  if (found == pool.end() || found->second == 0) return false;
  --found->second;
  return true;
}

} // namespace

std::expected<StatementFile, std::string> read_statement_file(fs::path const& file) {
  std::ifstream in(file, std::ios::binary);
  if (!in) return std::unexpected(std::format("Kan inte läsa {}", file.string()));
  std::vector<std::uint8_t> const bytes{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
  auto document = statement::parse_statement(bytes);
  if (!document) return std::unexpected(std::format("{}: {}", file.string(), document.error()));
  return StatementFile{file, std::move(*document)};
}

std::optional<AccountGuess> guess_statement_account(books::Books const& books, statement::Document const& document) {
  std::map<std::string, Pool> pools;
  for (auto const& year : books.years) {
    for (auto const& voucher : year.vouchers) {
      for (auto const& posting : voucher.postings) {
        if (posting.kind == books::Posting::Kind::removed) continue;
        ++pools[posting.account][{posting.date.value_or(voucher.event.date), posting.amount}];
      }
    }
  }
  std::optional<AccountGuess> best;
  bool tie = false;
  for (auto& [account, pool] : pools) {
    std::size_t matches = 0;
    for (auto const& entry : document.entries) matches += take(pool, {entry.date, entry.amount}) ? 1 : 0;
    if (matches == 0) continue;
    if (!best || matches > best->matches) {
      best = AccountGuess{account, matches};
      tie = false;
    } else if (matches == best->matches) {
      tie = true;
    }
  }
  if (tie) return std::nullopt;
  return best;
}

StatementDraftPlan plan_statement_drafts(books::FiscalYear const& year, statement::Document const& document,
                                         std::string account) {
  Pool booked;
  for (auto const& voucher : year.vouchers) {
    for (auto const& posting : voucher.postings) {
      if (posting.account != account || posting.kind == books::Posting::Kind::removed) continue;
      ++booked[{posting.date.value_or(voucher.event.date), posting.amount}];
    }
  }
  Pool drafted;
  for (auto const& draft : year.drafts) {
    for (auto const& posting : draft.postings) {
      if (posting.account != account || posting.kind == books::Posting::Kind::removed) continue;
      ++drafted[{posting.date.value_or(draft.event.date), posting.amount}];
    }
  }
  StatementDraftPlan plan;
  plan.account = std::move(account);
  for (auto const& entry : document.entries) {
    Key const key{entry.date, entry.amount};
    if (!year.holds(entry.date)) {
      plan.outside.push_back(entry);
    } else if (take(booked, key)) {
      plan.booked.push_back(entry);
    } else if (take(drafted, key)) {
      plan.drafted.push_back(entry);
    } else {
      plan.added.push_back(entry);
    }
  }
  return plan;
}

std::string statement_source(fs::path const& project_folder, fs::path const& file, std::size_t line) {
  return std::format("{}:{}", stored_path(file, project_folder), line);
}

std::expected<std::size_t, std::string> add_statement_drafts(fs::path const& project_folder, fs::path const& file,
                                                             StatementDraftPlan const& plan, Date year_start,
                                                             Date today) {
  auto books = books::read_books(project_folder);
  if (!books) return std::unexpected(books.error());
  // A year after the last in the books is added: the books run on over the years
  auto* const year = books::continued_year(*books, year_start);
  if (year == nullptr) {
    return std::unexpected(std::format("Räkenskapsåret som börjar {} finns inte i bokföringen", format_iso_date(year_start)));
  }
  for (auto const& entry : plan.added) {
    books::Draft draft;
    draft.event.date = entry.date;
    draft.noted = today;
    books::Posting posting;
    posting.account = plan.account;
    posting.amount = entry.amount;
    posting.date = entry.date;
    posting.text = entry.text;
    posting.source = statement_source(project_folder, file, entry.line);
    draft.postings.push_back(std::move(posting));
    year->place_draft(std::move(draft));
  }
  if (auto written = books::write_books(project_folder, *books); !written) return std::unexpected(written.error());
  if (auto recorded = record_statement_use(project_folder, file, year->start, year->end, today); !recorded) {
    return std::unexpected(recorded.error());
  }
  return plan.added.size();
}

} // namespace cratchit
