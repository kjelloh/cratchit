#include "core/ink2/sru_output.hpp"

#include "core/ink2/previous.hpp"

#include <algorithm>
#include <cctype>
#include <format>
#include <fstream>
#include <iterator>
#include <system_error>

namespace cratchit::ink2 {

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

std::string trimmed(std::string_view text) {
  auto const first = text.find_first_not_of(" \t\r");
  if (first == std::string_view::npos) {
    return "";
  }
  auto const last = text.find_last_not_of(" \t\r");
  return std::string(text.substr(first, last - first + 1));
}

void add_block(std::string& out, std::string_view form, std::vector<SkvField> const& skv, Filer const& filer,
               sie::FiscalYear const& year, Stamp const& stamp, std::map<std::string, Kronor> const& amounts,
               std::set<std::string> const& marks) {
  out += std::format("#BLANKETT {}\n", block_name(form, year.end));
  out += std::format("#IDENTITET {} {} {}\n", sru_orgnr(filer.orgnr), format_compact_date(stamp.date), stamp.time);
  out += std::format("#NAMN {}\n", filer.name);
  out += std::format("#UPPGIFT 7011 {}\n", format_compact_date(year.start));
  out += std::format("#UPPGIFT 7012 {}\n", format_compact_date(year.end));
  for (auto const& field : fields_of(skv, form)) {
    if (field.type == SkvField::Type::mark) {
      if (marks.contains(field.sru)) {
        out += std::format("#UPPGIFT {} X\n", field.sru);
      }
    } else if (auto const amount = amounts.find(field.sru); amount != amounts.end()) {
      out += std::format("#UPPGIFT {} {}\n", field.sru, amount->second);
    }
  }
  out += "#BLANKETTSLUT\n";
}

} // namespace

std::string sru_orgnr(std::string_view orgnr) {
  return "16" + orgnr_digits(orgnr);
}

std::string block_name(std::string_view form, Date end) {
  auto const month = static_cast<unsigned>(end.month());
  auto const period = month <= 4 ? 1 : month <= 6 ? 2 : month <= 8 ? 3 : 4;
  return std::format("{}-{}P{}", form, static_cast<int>(end.year()), period);
}

std::optional<std::pair<std::string, std::string>> postal_of(std::string_view address) {
  std::string postnr;
  std::size_t position = 0;
  while (position < address.size() && postnr.size() < 5) {
    auto const c = address[position];
    if (c >= '0' && c <= '9') {
      postnr += c;
    } else if (c != ' ' || postnr.empty()) {
      return std::nullopt;
    }
    ++position;
  }
  auto const postort = trimmed(address.substr(position));
  if (postnr.size() != 5 || postort.empty() || (position < address.size() && address[position] != ' ')) {
    return std::nullopt;
  }
  return std::pair{postnr, postort};
}

std::optional<std::pair<std::string, std::string>> postal_of_info(std::string_view info) {
  std::string postnr;
  std::string postort;
  while (!info.empty()) {
    auto const end = info.find('\n');
    auto const line = info.substr(0, end);
    info = end == std::string_view::npos ? std::string_view{} : info.substr(end + 1);
    if (line.starts_with("#POSTNR ")) {
      postnr = trimmed(line.substr(8));
    } else if (line.starts_with("#POSTORT ")) {
      postort = trimmed(line.substr(9));
    }
  }
  if (postnr.size() != 5 || !std::ranges::all_of(postnr, [](char c) { return c >= '0' && c <= '9'; }) ||
      postort.empty()) {
    return std::nullopt;
  }
  return std::pair{postnr, postort};
}

std::optional<std::filesystem::path> info_file_in(std::filesystem::path const& folder) {
  std::error_code error;
  for (auto const& entry : std::filesystem::directory_iterator(folder, error)) {
    auto name = entry.path().filename().string();
    std::ranges::transform(name, name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (entry.is_regular_file() && (name == "info.sru" || name == "infosru")) {
      return entry.path();
    }
  }
  return std::nullopt;
}

std::string format_blanketter(std::vector<SkvField> const& skv, Filer const& filer, sie::FiscalYear const& year,
                              Form const& form, TaxReturn const& tax, Stamp const& stamp) {
  std::map<std::string, Kronor> ink2r;
  for (auto const& [sru, field] : form.fields) {
    if (field.used && form_of(sru) == "INK2R") {
      ink2r[sru] = field.kronor;
    }
  }
  std::string out;
  add_block(out, "INK2", skv, filer, year, stamp, tax.ink2, {});
  add_block(out, "INK2R", skv, filer, year, stamp, ink2r, {});
  add_block(out, "INK2S", skv, filer, year, stamp, tax.ink2s, tax.ink2s_marks);
  out += "#FIL_SLUT\n";
  return out;
}

std::string format_info(Filer const& filer, Stamp const& stamp, std::string_view program) {
  std::string out;
  out += "#DATABESKRIVNING_START\n";
  out += "#PRODUKT SRU\n";
  out += std::format("#SKAPAD {} {}\n", format_compact_date(stamp.date), stamp.time);
  out += std::format("#PROGRAM {}\n", program);
  out += "#FILNAMN BLANKETTER.SRU\n";
  out += "#DATABESKRIVNING_SLUT\n";
  out += "#MEDIELEV_START\n";
  out += std::format("#ORGNR {}\n", sru_orgnr(filer.orgnr));
  out += std::format("#NAMN {}\n", filer.name);
  out += std::format("#POSTNR {}\n", filer.postnr);
  out += std::format("#POSTORT {}\n", filer.postort);
  out += "#MEDIELEV_SLUT\n";
  return out;
}

std::expected<std::string, std::string> to_latin1(std::string_view utf8) {
  std::string out;
  for (std::size_t index = 0; index < utf8.size();) {
    auto const lead = static_cast<unsigned char>(utf8[index]);
    if (lead < 0x80) {
      out += static_cast<char>(lead);
      ++index;
      continue;
    }
    // Two bytes, C2 or C3 then a continuation byte, are U+0080 to U+00FF; anything else is outside ISO 8859-1
    if ((lead == 0xC2 || lead == 0xC3) && index + 1 < utf8.size() &&
        (static_cast<unsigned char>(utf8[index + 1]) & 0xC0) == 0x80) {
      out += static_cast<char>(((lead & 0x03) << 6) | (static_cast<unsigned char>(utf8[index + 1]) & 0x3F));
      index += 2;
      continue;
    }
    auto length = std::size_t{1};
    while (index + length < utf8.size() && (static_cast<unsigned char>(utf8[index + length]) & 0xC0) == 0x80) {
      ++length;
    }
    return std::unexpected(std::format("\"{}\" is not in ISO 8859-1, which SRU files are", utf8.substr(index, length)));
  }
  return out;
}

OutputPaths output_paths(std::filesystem::path const& base, std::string_view orgnr, sie::FiscalYear const& year) {
  std::string name;
  for (char const c : orgnr) {
    bool const keep = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '-';
    name += keep ? c : '_';
  }
  auto const stem = std::format("{}_{}_{}", name, format_iso_date(year.start), format_iso_date(year.end));
  auto const folder = base / stem;
  return {folder,
          folder / std::filesystem::path(std::u8string(stem.begin(), stem.end()) + u8"_ink2_fält.txt"),
          folder / (stem + "_ink2_rapport.txt"),
          folder / "BLANKETTER.SRU",
          folder / "INFO.SRU",
          folder / (stem + ".se")};
}

std::expected<Written, std::string> write_outputs(OutputPaths const& paths, std::string const& report,
                                                  std::optional<std::string> const& fields,
                                                  std::optional<SruFiles> const& sru,
                                                  std::filesystem::path const& sie_input) {
  std::optional<std::pair<std::string, std::string>> sru_latin1;
  if (sru) {
    auto const blanketter = to_latin1(sru->blanketter);
    if (!blanketter) {
      return std::unexpected(std::format("BLANKETTER.SRU: {}", blanketter.error()));
    }
    auto const info = to_latin1(sru->info);
    if (!info) {
      return std::unexpected(std::format("INFO.SRU: {}", info.error()));
    }
    sru_latin1 = std::pair{*blanketter, *info};
  }
  auto const input = read_file(sie_input);
  if (!input) {
    return std::unexpected(std::format("Cannot read {}", sie_input.string()));
  }
  std::error_code error;
  std::filesystem::create_directories(paths.folder, error);
  if (error) {
    return std::unexpected(std::format("Cannot create the folder {}: {}", paths.folder.string(), error.message()));
  }
  Written written;
  if (auto const done = write_file(paths.report, report); !done) {
    return std::unexpected(done.error());
  }
  if (fields) {
    if (auto const done = write_file(paths.fields, *fields); !done) {
      return std::unexpected(done.error());
    }
    written.fields = true;
  }
  if (sru_latin1) {
    if (auto const done = write_file(paths.blanketter, sru_latin1->first); !done) {
      return std::unexpected(done.error());
    }
    if (auto const done = write_file(paths.info, sru_latin1->second); !done) {
      return std::unexpected(done.error());
    }
    written.sru = true;
  } else {
    written.old_sru = std::filesystem::exists(paths.blanketter, error) || std::filesystem::exists(paths.info, error);
  }
  // The copy goes with the return as text: without a new one, an earlier copy stays to match the earlier one
  auto const existing = read_file(paths.sie);
  if (existing != input && (fields || !existing)) {
    if (auto const done = write_file(paths.sie, *input); !done) {
      return std::unexpected(done.error());
    }
    written.sie_copied = true;
  }
  return written;
}

} // namespace cratchit::ink2
