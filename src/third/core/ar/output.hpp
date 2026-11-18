#pragma once

#include "core/sie/document.hpp"

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit::ar {

// Where one årsredovisning goes: a folder named <orgnr>_<start>_<end> holding
// the årsredovisning, its report and the SIE file it was made from, all named
// after the folder so that each says what it is on its own
struct OutputPaths {
  std::filesystem::path folder; // <base>/<stem>
  std::filesystem::path ar;     // <stem>_arsredovisning.txt
  std::filesystem::path report; // <stem>_rapport.txt
  std::filesystem::path sie;    // <stem>.se
};

// E.g. 556782-8172_2025-05-01_2026-04-30. Characters other than letters, digits
// and '-' in the orgnr become '_'.
std::string output_stem(std::string_view orgnr, sie::FiscalYear const& year);

OutputPaths output_paths(std::filesystem::path const& base, std::string_view orgnr, sie::FiscalYear const& year);

// What write_outputs did
struct Written {
  bool ar = false;          // the årsredovisning was written (replacing any earlier one)
  bool sie_copied = false;  // the SIE file was copied (missing, or different with a new årsredovisning)
};

// Creates the folder if missing and writes the report, the årsredovisning when
// given (on errors there is none, and an earlier one is left as it is), and a
// copy of the SIE file sie_input unless an identical one is already there. The
// copy goes with the årsredovisning: without one, an earlier copy is left so that
// it still matches the earlier årsredovisning. Every file written replaces the one
// before. Error text names the file.
std::expected<Written, std::string> write_outputs(OutputPaths const& paths, std::optional<std::string> const& ar,
                                                  std::string const& report, std::filesystem::path const& sie_input);

// What the report says besides the K2 check
struct ReportHeader {
  std::string ar_file;      // the årsredovisning's file name
  std::string generated;    // when, e.g. 2026-11-02 18:04
  std::string program;      // e.g. cratchit 0.1.0 --sie-to-ar
  std::string sie_input;    // the SIE file given
  std::string sie_copy;     // its copy's file name
  std::string sie_details;  // what the SIE file says of itself, e.g. #PROGRAM and #GEN
  std::string mapping;      // the mapping used
  std::string status;       // e.g. "PRELIMINÄR: the books are not closed; 10 items TBD"
};

// The report: the header, the errors, the notes, then check (the K2 check as text)
std::string format_report(ReportHeader const& header, std::vector<std::string> const& errors,
                          std::vector<std::string> const& notes, std::string const& check);

} // namespace cratchit::ar
