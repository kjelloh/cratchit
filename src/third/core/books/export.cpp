#include "core/books/export.hpp"

#include "core/bas.hpp"
#include "core/version.hpp"

#include <format>
#include <fstream>
#include <set>
#include <string>

namespace cratchit::books {

namespace {

sie::Transaction::Kind kind_of(Posting::Kind kind) {
  switch (kind) {
  case Posting::Kind::added:
    return sie::Transaction::Kind::added;
  case Posting::Kind::removed:
    return sie::Transaction::Kind::removed;
  default:
    return sie::Transaction::Kind::normal;
  }
}

sie::Voucher voucher_of(Voucher const& voucher) {
  sie::Voucher result{voucher.series, voucher.number, voucher.event.date, voucher.event.heading,
                      voucher.registered, voucher.signature, {}, 0};
  for (auto const& p : voucher.postings) {
    result.transactions.push_back({kind_of(p.kind), p.account, p.objects, p.amount, p.date, p.text, p.quantity,
                                   p.signature});
  }
  return result;
}

// #IB and #UB of the balance accounts where either is not zero; #RES of the other accounts not at zero
// (result accounts, and those BAS gives no kind, such as the 9xxx memo accounts, as Fortnox does)
void add_balances(Books const& books, YearBalances const& year, int index, sie::Document& document) {
  std::set<std::string> accounts;
  for (auto const& [account, amount] : year.opening) {
    accounts.insert(account);
  }
  for (auto const& [account, amount] : year.closing) {
    accounts.insert(account);
  }
  auto const amount_in = [](std::map<std::string, Amount> const& amounts, std::string const& account) {
    auto const it = amounts.find(account);
    return it == amounts.end() ? Amount{0} : it->second;
  };
  for (auto const& account : accounts) {
    if (account_kind(books, account) == AccountKind::balance) {
      auto const opening = amount_in(year.opening, account);
      auto const closing = amount_in(year.closing, account);
      if (opening != 0 || closing != 0) {
        document.opening.push_back({index, account, opening, {}});
        document.closing.push_back({index, account, closing, {}});
      }
    }
  }
  for (auto const& [account, amount] : year.change) {
    if (account_kind(books, account) != AccountKind::balance && amount != 0) {
      document.results.push_back({index, account, amount, {}});
    }
  }
}

} // namespace

sie::Document export_document(Books const& books, Identity const& identity, std::size_t index, Date generated,
                              std::optional<std::vector<std::size_t>> const& only) {
  sie::Document document;
  auto const& year = books.years[index];
  document.flag = "0";
  document.format = "PC8";
  document.sie_type = "4";
  document.program = "cratchit";
  document.program_version = std::string(version());
  document.generated = generated;
  document.company_id = books.company_id;
  document.company_name = identity.name;
  document.orgnr = identity.orgnr;
  document.address = {books.address.contact, books.address.street, books.address.postal, books.address.phone};
  document.years.push_back({0, year.start, year.end});
  if (index > 0) {
    document.years.push_back({-1, books.years[index - 1].start, books.years[index - 1].end});
  }
  document.covers_until = year.end;
  document.chart_type = books.chart_type;
  for (auto const& account : books.accounts) {
    document.accounts.push_back({account.number, account.name, account.type, account.sru, account.unit});
  }
  for (auto const& dimension : books.dimensions) {
    document.dimensions.push_back({dimension.number, dimension.name, dimension.super});
  }
  for (auto const& object : books.objects) {
    document.objects.push_back({object.dimension, object.number, object.name});
  }
  if (only) {
    for (auto const i : *only) {
      document.vouchers.push_back(voucher_of(year.vouchers[i]));
    }
    return document;
  }
  auto const all = balances(books);
  add_balances(books, all[index], 0, document);
  if (index > 0) {
    add_balances(books, all[index - 1], -1, document);
  }
  for (auto const& voucher : year.vouchers) {
    document.vouchers.push_back(voucher_of(voucher));
  }
  return document;
}

std::string export_file_name(FiscalYear const& year, ExportKind kind, int index) {
  auto const start = static_cast<int>(year.start.year());
  auto const end = static_cast<int>(year.end.year());
  auto const name = std::format("{}_{}",
                                start == end ? std::format("sie_{}", start) : std::format("sie_{}-{:02}", start, end % 100),
                                kind == ExportKind::all ? "allt" : "skillnad");
  return index < 2 ? name + ".se" : std::format("{}_{}.se", name, index);
}

std::filesystem::path free_export_file(std::filesystem::path const& folder, FiscalYear const& year, ExportKind kind) {
  for (int index = 1;; ++index) {
    auto path = folder / export_file_name(year, kind, index);
    std::error_code error;
    if (!std::filesystem::exists(path, error)) {
      return path;
    }
  }
}

std::expected<std::filesystem::path, std::string> write_export(std::filesystem::path const& folder,
                                                               FiscalYear const& year, ExportKind kind,
                                                               std::span<std::uint8_t const> bytes) {
  auto const path = free_export_file(folder, year, kind);
  std::ofstream out(path, std::ios::binary);
  out.write(reinterpret_cast<char const*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  out.close();
  if (!out) {
    return std::unexpected(std::format("Could not write {}", path.string()));
  }
  return path;
}

std::expected<ExportDiff, std::string> diff_with(Books const& books, std::size_t index, Identity const& identity,
                                                 sie::Document const& document) {
  auto const& year = books.years[index];
  if (!identity.orgnr.empty() && document.orgnr != identity.orgnr) {
    return std::unexpected(std::format("The file is for organisation number {}, but the project {} has {}",
                                       document.orgnr, identity.name, identity.orgnr));
  }
  auto const file_year = document.year(0);
  if (!file_year || file_year->start != year.start || file_year->end != year.end) {
    return std::unexpected(std::format("The file's fiscal year 0 is {}, not {} - {}",
                                       file_year ? std::format("{} - {}", format_iso_date(file_year->start),
                                                               format_iso_date(file_year->end))
                                                 : std::string{"missing"},
                                       format_iso_date(year.start), format_iso_date(year.end)));
  }
  ExportDiff diff;
  std::vector<bool> matched(year.vouchers.size(), false);
  for (auto const& record : document.vouchers) {
    auto const voucher = books::voucher_of(record);
    auto const rows = counted_rows(voucher);
    auto found = false;
    for (std::size_t i = 0; i < year.vouchers.size() && !found; ++i) {
      if (!matched[i] && year.vouchers[i].event.date == voucher.event.date && counted_rows(year.vouchers[i]) == rows) {
        matched[i] = true;
        found = true;
      }
    }
    if (!found) {
      diff.file_only.push_back(voucher);
    }
  }
  for (std::size_t i = 0; i < year.vouchers.size(); ++i) {
    if (!matched[i]) {
      diff.books_only.push_back(i);
    }
  }
  return diff;
}

} // namespace cratchit::books
