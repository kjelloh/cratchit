#pragma once

#include "core/amount.hpp"
#include "core/date.hpp"

#include <cstddef>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace cratchit {

// What an SIE file holds, in short, so that the user can tell files apart
// whose names say nothing (e.g. when looking one up with Tab)
struct SieSummary {
  std::optional<std::pair<Date, Date>> year;          // #RAR 0
  std::optional<Date> generated;                      // #GEN
  std::size_t vouchers = 0;                           // #VER
  std::optional<Date> last_date;                      // the latest voucher date
  std::vector<std::pair<std::string, std::string>> series; // each series and its highest number, by series
  Amount balance = 0;         // the balance accounts (1xxx, 2xxx) summed at the end: #IB 0 plus the rows; 0 when they balance
  bool result_booked = false; // a row on 8999 (Årets resultat), as the closing books the year's result
};

// What the file says about its fiscal year (#RAR 0), as of today:
// * ongoing ("pågående"): the year has not ended.
// * incomplete ("ofullbordat"): ended, but the balance sheet does not balance
//   and no result is booked on 8999, e.g. no closing yet, or a file made
//   before the year ended. warning when the year is older than last year
//   (it ended more than a year before today), since such a year must be complete.
// * complete ("fullbordat"): the balance sheet balances (and the checks to
//   come pass).
// * mismatch ("misstämmer"): a result is booked on 8999, but the balance
//   sheet does not balance.
enum class YearStatus { ongoing, incomplete, complete, mismatch };

struct SieStatus {
  YearStatus status = YearStatus::ongoing;
  bool warning = false;
};

// The status of the summarized file's fiscal year on day today; nothing when the file has no #RAR 0
std::optional<SieStatus> status_of(SieSummary const& summary, Date today);

// "pågående", "ofullbordat", "fullbordat" or "misstämmer", and " !" after it with a warning
std::string status_text(SieStatus status);

// Reads and summarizes the SIE file. Does not log, since it is read each time
// the user lists files. Error text when the file cannot be read or is no SIE
// file (#SIETYP missing); other errors in the file are left to whoever imports it.
std::expected<SieSummary, std::string> summarize_sie_file(std::filesystem::path const& file);

// The summary on one line, with the status on day today, e.g.
// "2025-05-01 – 2026-04-30  fullbordat  skapad 2026-11-12  118 ver. t.o.m. 2026-04-30  A 82 · D 11 · M 23".
// Parts the file lacks are left out.
std::string summary_line(SieSummary const& summary, Date today);

// Today, as cratchit takes it (see today_iso)
Date today_date();

// The lines to show under an SIE file's name: its summary line, or why it cannot be read
std::vector<std::string> describe_sie_file(std::filesystem::path const& file);

// The SIE files and folders found with Tab, laid out to choose from: a heading
// per fiscal year (#RAR 0, latest first), and under it a line per file in
// columns: name, status (see status_of, as of today), created (#GEN), vouchers and the latest date, and the highest
// number per series. Within a year the latest created file comes first. Then
// "Utan räkenskapsår", "Går inte att läsa" (with the reason) and "Mappar".
// Headings are not indented; their lines are indented two spaces.
std::vector<std::string> list_sie_files(std::vector<std::filesystem::path> const& files,
                                        std::vector<std::filesystem::path> const& folders);

} // namespace cratchit
