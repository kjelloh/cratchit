#include "core/ink2/report.hpp"

#include <format>
#include <string_view>

namespace cratchit::ink2 {

namespace {

// A line under text as wide as it shows: one character per code point of UTF-8 text
std::string underline(std::string_view text, char c) {
  std::size_t width = 0;
  for (char const byte : text) {
    width += (static_cast<unsigned char>(byte) & 0xC0) != 0x80;
  }
  return std::string(width, c);
}

void add_section(std::string& out, std::string_view title, std::vector<std::string> const& lines) {
  out += std::format("\n{}\n{}\n", title, underline(title, '-'));
  if (lines.empty()) {
    out += "None.\n";
  }
  for (auto const& line : lines) {
    out += line + '\n';
  }
}

} // namespace

std::string format_report(ReportHeader const& header, std::vector<std::string> const& errors,
                          std::vector<std::string> const& notes, std::string const& check) {
  auto const title = std::format("Report for {}", header.fields_file);
  std::string out = std::format("{}\n{}\n\n", title, underline(title, '='));
  out += std::format("Generated: {} by {}\n", header.generated, header.program);
  out += std::format("SIE file:  {}, copied as {}\n", header.sie_input, header.sie_copy);
  if (!header.sie_details.empty()) {
    out += std::format("           {}\n", header.sie_details);
  }
  out += std::format("Last year: {}\n", header.last_year);
  out += std::format("Mapping:   {}\n", header.mapping);
  out += std::format("Status:    {}\n", header.status);
  out += std::format("SRU files: {}\n", header.sru);
  add_section(out, "Errors", errors);
  add_section(out, "Notes", notes);
  out += '\n' + check;
  return out;
}

} // namespace cratchit::ink2
