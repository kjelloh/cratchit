#pragma once

#include "core/books/books.hpp"
#include "core/books/import.hpp"
#include "core/date.hpp"
#include "core/sie/document.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

// Exporting a fiscal year of the books as an SIE 4E file: the inverse of the import
namespace cratchit::books {

// The SIE 4E document of the fiscal year at index (into books.years) as year 0,
// and of the year before in the books as -1, as Fortnox exports them:
// identification (cratchit as the program, generated on the given day, the
// project's name and orgnr, the books' company id, address and chart type,
// #OMFATTN the year's end), every account, dimension and object, the balances
// cratchit calculates (see balances): #IB and #UB of each balance account where
// either is not zero, #RES of each other account (result accounts, and the
// 9xxx memo accounts BAS gives no kind) where it is not zero, then
// the year's vouchers (year 0 only) with their postings as #TRANS, #RTRANS
// (added) and #BTRANS (removed). Drafts are not exported; nor are
// object or period balances, which the books do not keep.
// With only (indexes into the year's vouchers), a part of the year: just those
// vouchers, and no balances (#IB, #UB, #RES), so no program takes balances from it.
sie::Document export_document(Books const& books, Identity const& identity, std::size_t index, Date generated,
                              std::optional<std::vector<std::size_t>> const& only = std::nullopt);

// What an export holds: the whole year with balances ("Allt"), or the vouchers not in a chosen SIE file
// ("Skillnad", see diff_with)
enum class ExportKind { all, diff };

// The file name of a fiscal year's export, named as its prompt and kind: sie_2026-27_allt.se for
// a year from 2026 into 2027, sie_2025_skillnad.se for a calendar year; with index 2 or
// more sie_2026-27_allt_2.se
std::string export_file_name(FiscalYear const& year, ExportKind kind, int index = 1);

// The first export_file_name of the year and kind in the folder that is not taken
std::filesystem::path free_export_file(std::filesystem::path const& folder, FiscalYear const& year, ExportKind kind);

// Writes the bytes to the first export_file_name of the year and kind in the folder that
// is not taken, so an earlier export is never overwritten; the file written, or error text
std::expected<std::filesystem::path, std::string> write_export(std::filesystem::path const& folder,
                                                               FiscalYear const& year, ExportKind kind,
                                                               std::span<std::uint8_t const> bytes);

// A fiscal year of the books compared with an SIE file of the same year: each voucher of the file is matched
// to one voucher of the books with the same date and counted postings (account and amount), whatever its
// series and number, as the import's renamed vouchers
struct ExportDiff {
  std::vector<std::size_t> books_only; // the year's vouchers not in the file, indexes into its vouchers
  std::vector<Voucher> file_only;      // the file's vouchers not in the books
};

// The diff of the fiscal year at index with an SIE document; error text when the document is for another
// organisation number (when the project has one) or has another fiscal year 0
std::expected<ExportDiff, std::string> diff_with(Books const& books, std::size_t index, Identity const& identity,
                                                 sie::Document const& document);

} // namespace cratchit::books
