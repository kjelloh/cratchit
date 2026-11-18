#pragma once

#include "core/date.hpp"
#include "core/ink2/form.hpp"
#include "core/ink2/skv.hpp"
#include "core/ink2/tax.hpp"
#include "core/sie/document.hpp"

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cratchit::ink2 {

// When the SRU files are made: #IDENTITET and #SKAPAD
struct Stamp {
  Date date;
  std::string time; // HHMMSS
};

// Who files: INFO.SRU's #MEDIELEV block
struct Filer {
  std::string orgnr; // as the books give it, e.g. 556782-8172
  std::string name;
  std::string postnr; // five digits
  std::string postort;
};

// The orgnr as SRU wants it: 16 and its ten digits, e.g. 165567828172
std::string sru_orgnr(std::string_view orgnr);

// The block name of a form for a fiscal year ending end: the form, the year it ends and the
// period by its last day (P1 to 04-30, P2 to 06-30, P3 to 08-31, P4 to 12-31), e.g. INK2R-2026P1
std::string block_name(std::string_view form, Date end);

// Postnummer and postort of a postal address such as "17141 Solna" or "176 68 Järfälla"
std::optional<std::pair<std::string, std::string>> postal_of(std::string_view address);

// Postnummer and postort of an INFO.SRU's text (#POSTNR and #POSTORT)
std::optional<std::pair<std::string, std::string>> postal_of_info(std::string_view info);

// The file INFO.SRU (or INFOSRU, in any case) in folder, if any
std::optional<std::filesystem::path> info_file_in(std::filesystem::path const& folder);

// BLANKETTER.SRU (as UTF-8 text): the blocks INK2, INK2R and INK2S, each with #IDENTITET, #NAMN,
// the fiscal year (7011, 7012) and the fields given in SKV's order (INK2R: the fields with an
// account; INK2S and INK2: those tax gives), then #FIL_SLUT. Records end with LF.
std::string format_blanketter(std::vector<SkvField> const& skv, Filer const& filer, sie::FiscalYear const& year,
                              Form const& form, TaxReturn const& tax, Stamp const& stamp);

// INFO.SRU (as UTF-8 text)
std::string format_info(Filer const& filer, Stamp const& stamp, std::string_view program);

// UTF-8 text as ISO 8859-1, as SRU files are. Error text for a character it does not have.
std::expected<std::string, std::string> to_latin1(std::string_view utf8);

// Where a return goes: a folder <stem> = <orgnr>_<start>_<end> holding the return as text
// (<stem>_ink2_fält.txt), its report (<stem>_ink2_rapport.txt), BLANKETTER.SRU, INFO.SRU and a
// copy of the SIE file (<stem>.se)
struct OutputPaths {
  std::filesystem::path folder;
  std::filesystem::path fields;
  std::filesystem::path report;
  std::filesystem::path blanketter;
  std::filesystem::path info;
  std::filesystem::path sie;
};

OutputPaths output_paths(std::filesystem::path const& base, std::string_view orgnr, sie::FiscalYear const& year);

// The SRU files' text (UTF-8)
struct SruFiles {
  std::string blanketter;
  std::string info;
};

// What write_outputs did
struct Written {
  bool fields = false;     // the return as text was written
  bool sru = false;        // the SRU files were written
  bool old_sru = false;    // earlier SRU files are left, not written now: they are of an earlier run
  bool sie_copied = false; // the SIE copy was written
};

// Creates the folder if missing and writes the report, the return as text when given (with errors
// there is none, and an earlier one is left as it is), and the SRU files when given (in ISO 8859-1).
// Each replaces the one before. The SIE file sie_input is copied unless an identical copy is there;
// the copy goes with the return as text: without one, an earlier copy is left so that the two
// still belong together. Error text names the file, or a character not in ISO 8859-1.
std::expected<Written, std::string> write_outputs(OutputPaths const& paths, std::string const& report,
                                                  std::optional<std::string> const& fields,
                                                  std::optional<SruFiles> const& sru,
                                                  std::filesystem::path const& sie_input);

} // namespace cratchit::ink2
