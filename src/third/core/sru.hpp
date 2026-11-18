#pragma once

#include <expected>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace cratchit {

// A form of an SRU file (SKV's BLANKETTER.SRU): #BLANKETT to #BLANKETTSLUT
struct SruForm {
  std::string name;                          // e.g. INK2R-2025P1
  std::string identity;                      // #IDENTITET: orgnr, date and time
  std::map<std::string, std::string> fields; // #UPPGIFT by its code, e.g. 7215 -> 65924

  // The name before its year, e.g. INK2R
  std::string_view kind() const;
};

// The forms of an SRU file. Other records (#NAMN, #SYSTEMINFO, #FIL_SLUT) are skipped.
// Error text names the line for an #UPPGIFT outside a form, a form not ended, or a field given twice.
std::expected<std::vector<SruForm>, std::string> parse_sru(std::string_view text);

// Read and parse a file. Error text names the file.
std::expected<std::vector<SruForm>, std::string> read_sru(std::filesystem::path const& file);

} // namespace cratchit
