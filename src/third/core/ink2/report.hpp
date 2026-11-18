#pragma once

#include <string>
#include <vector>

namespace cratchit::ink2 {

// What the report says before the errors, the notes and the INK2 check
struct ReportHeader {
  std::string fields_file; // the return's file name, e.g. <stem>_ink2_fält.txt
  std::string generated;   // when, e.g. 2026-11-03 18:04
  std::string program;     // e.g. cratchit 0.1.0 --sie-to-ink2
  std::string sie_input;   // the SIE file given
  std::string sie_copy;    // its copy's file name
  std::string sie_details; // what the SIE file says of itself, e.g. #PROGRAM and #GEN
  std::string last_year;   // where last year's underskott is from
  std::string mapping;     // the mapping files used
  std::string status;      // e.g. "not complete; 2 items TBD"
  std::string sru;         // what became of the SRU files
};

// The report: the header, the errors, the notes, then check (the INK2 check as text)
std::string format_report(ReportHeader const& header, std::vector<std::string> const& errors,
                          std::vector<std::string> const& notes, std::string const& check);

} // namespace cratchit::ink2
