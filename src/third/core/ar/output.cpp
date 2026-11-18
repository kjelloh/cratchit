#include "core/ar/output.hpp"

#include "core/date.hpp"

#include <format>
#include <fstream>
#include <iterator>
#include <system_error>

namespace cratchit::ar {

namespace {

std::optional<std::string> read_file(std::filesystem::path const& file) {
  std::ifstream in(file, std::ios::binary);
  if (!in) {
    return std::nullopt;
  }
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

std::expected<void, std::string> write_file(std::filesystem::path const& file, std::string_view text) {
  std::ofstream out(file, std::ios::binary | std::ios::trunc);
  if (!out || !out.write(text.data(), static_cast<std::streamsize>(text.size())) || !out.flush()) {
    return std::unexpected(std::format("Cannot write {}", file.string()));
  }
  return {};
}

void add_section(std::string& out, std::string_view title, std::vector<std::string> const& lines) {
  out += std::format("\n{}\n{}\n", title, std::string(title.size(), '-'));
  if (lines.empty()) {
    out += "None.\n";
  }
  for (auto const& line : lines) {
    out += line + '\n';
  }
}

} // namespace

std::string output_stem(std::string_view orgnr, sie::FiscalYear const& year) {
  std::string name;
  for (char const c : orgnr) {
    bool const keep = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '-';
    name += keep ? c : '_';
  }
  return std::format("{}_{}_{}", name, format_iso_date(year.start), format_iso_date(year.end));
}

OutputPaths output_paths(std::filesystem::path const& base, std::string_view orgnr, sie::FiscalYear const& year) {
  auto const stem = output_stem(orgnr, year);
  auto const folder = base / stem;
  return {folder, folder / (stem + "_arsredovisning.txt"), folder / (stem + "_rapport.txt"), folder / (stem + ".se")};
}

std::expected<Written, std::string> write_outputs(OutputPaths const& paths, std::optional<std::string> const& ar,
                                                  std::string const& report, std::filesystem::path const& sie_input) {
  std::error_code error;
  std::filesystem::create_directories(paths.folder, error);
  if (error) {
    return std::unexpected(std::format("Cannot create the folder {}: {}", paths.folder.string(), error.message()));
  }
  Written written;
  if (auto const done = write_file(paths.report, report); !done) {
    return std::unexpected(done.error());
  }
  if (ar) {
    if (auto const done = write_file(paths.ar, *ar); !done) {
      return std::unexpected(done.error());
    }
    written.ar = true;
  }
  auto const input = read_file(sie_input);
  if (!input) {
    return std::unexpected(std::format("Cannot read {}", sie_input.string()));
  }
  // The copy goes with the årsredovisning: without a new one, an earlier copy stays to match the earlier one
  auto const existing = read_file(paths.sie);
  if (existing != input && (ar || !existing)) {
    if (auto const done = write_file(paths.sie, *input); !done) {
      return std::unexpected(done.error());
    }
    written.sie_copied = true;
  }
  return written;
}

std::string format_report(ReportHeader const& header, std::vector<std::string> const& errors,
                          std::vector<std::string> const& notes, std::string const& check) {
  auto const title = std::format("Report for {}", header.ar_file);
  std::string out = std::format("{}\n{}\n\n", title, std::string(title.size(), '='));
  out += std::format("Generated: {} by {}\n", header.generated, header.program);
  out += std::format("SIE file:  {}, copied as {}\n", header.sie_input, header.sie_copy);
  if (!header.sie_details.empty()) {
    out += std::format("           {}\n", header.sie_details);
  }
  out += std::format("Mapping:   {}\n", header.mapping);
  out += std::format("Status:    {}\n", header.status);
  out += "\nThe årsredovisning is replaced on every run. Copy it before editing it, and merge your\n"
         "edits into a new version by hand.\n";
  add_section(out, "Errors", errors);
  add_section(out, "Notes", notes);
  if (!check.empty()) {
    out += '\n' + check;
  }
  return out;
}

} // namespace cratchit::ar
