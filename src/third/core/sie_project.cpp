#include "core/sie_project.hpp"

#include "core/details_project.hpp"
#include "core/inventory.hpp"

#include "core/path_value.hpp"
#include "core/sie_import.hpp"
#include "core/sie/document.hpp"
#include "core/sie/file.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <format>
#include <random>
#include <system_error>

namespace cratchit {

namespace fs = std::filesystem;

namespace {

// At most this many errors are given when a file has errors
constexpr std::size_t shown_errors = 3;

std::string errors_text(std::vector<std::string> const& errors) {
  std::string text = std::format("SIE-filen har {} fel:", errors.size());
  for (std::size_t i = 0; i < std::min(errors.size(), shown_errors); ++i) text += " " + errors[i] + ".";
  if (errors.size() > shown_errors) text += " ...";
  return text;
}

} // namespace

bool is_folder_name(std::string_view name) {
  if (name.empty() || name == "." || name == "..") return false;
  if (name.back() == ' ' || name.back() == '.') return false;
  for (auto const c : name) {
    if (static_cast<unsigned char>(c) < 0x20 || std::string_view("/\\:*?\"<>|").find(c) != std::string_view::npos) {
      return false;
    }
  }
  return true;
}

std::string new_uuid() {
  std::random_device device;
  std::seed_seq seed{device(), device(), device(), device(), device(), device(), device(), device()};
  std::mt19937_64 engine(seed);
  auto high = engine();
  auto low = engine();
  high = (high & ~std::uint64_t{0xF000}) | 0x4000;                        // version 4
  low = (low & ~(std::uint64_t{0xC} << 60)) | std::uint64_t{0x8} << 60; // variant 10xx
  return std::format("{:08x}-{:04x}-{:04x}-{:04x}-{:012x}", high >> 32, (high >> 16) & 0xFFFF, high & 0xFFFF,
                     low >> 48, low & 0xFFFF'FFFF'FFFF);
}

bool is_uuid_like(std::string_view text) {
  if (text.size() != 36) return false;
  for (std::size_t i = 0; i < text.size(); ++i) {
    auto const c = text[i];
    if (i == 8 || i == 13 || i == 18 || i == 23) {
      if (c != '-') return false;
    } else if (!std::isxdigit(static_cast<unsigned char>(c))) {
      return false;
    }
  }
  return true;
}

bool is_sie_file_name(fs::path const& file) {
  auto extension = file.extension().string();
  std::ranges::transform(extension, extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return extension == ".se" || extension == ".si" || extension == ".sie";
}

std::expected<SieProjectPlan, std::string> plan_project_from_sie(fs::path const& sie_file,
                                                                 std::optional<fs::path> const& workspace_folder,
                                                                 std::optional<fs::path> const& home,
                                                                 std::optional<fs::path> const& here) {
  std::error_code ec;
  if (!sie_file.is_absolute()) return std::unexpected(std::format("{} är inte en hel sökväg", sie_file.string()));
  if (!fs::is_regular_file(sie_file, ec)) return std::unexpected(std::format("{} är ingen fil", sie_file.string()));
  auto const lexed = sie::lex_file(sie_file);
  if (!lexed) return std::unexpected(lexed.error());
  auto const parsed = sie::parse_document(lexed->lines);
  auto const& document = parsed.document;
  if (document.sie_type.empty()) return std::unexpected(std::format("{} är ingen SIE-fil (#SIETYP saknas)", sie_file.string()));
  if (!parsed.errors.empty()) return std::unexpected(errors_text(parsed.errors));
  if (document.company_name.empty()) return std::unexpected("SIE-filen saknar företagets namn (#FNAMN)");
  if (!document.year(0)) return std::unexpected("SIE-filen saknar räkenskapsår (#RAR 0)");
  // The books are made here, or else in the SIE file's own folder; never in a new folder (beslut.md)
  auto const folder = here ? *here : sie_file.parent_path();
  if (auto const allowed = here ? check_project_here(folder, workspace_folder, home)
                                : check_project_folder(folder, workspace_folder, home, "SIE-filens mapp",
                                                       "Lägg SIE-filen i bokföringens egen mapp.");
      !allowed) {
    return std::unexpected(allowed.error());
  }
  SieProjectPlan plan;
  plan.sie_file = sie_file;
  plan.folder = folder;
  plan.name = document.company_name;
  plan.orgnr = document.orgnr;
  plan.alert = lexed->alert;
  plan.program = document.program_version.empty() ? document.program
                                                  : std::format("{} {}", document.program, document.program_version);
  plan.generated = document.generated;
  books::Books empty;
  empty.series = books::default_series(); // a new project starts with Fortnox's series
  plan.import = books::plan_import(empty, {plan.name, {}}, document);
  if (!plan.import.errors.empty()) return std::unexpected(errors_text(plan.import.errors));
  return plan;
}

std::expected<Project, std::string> create_project_from_sie(SieProjectPlan const& plan, std::string_view today) {
  std::error_code ec;
  if (!fs::create_directory(plan.folder / ".cratchit", ec)) {
    return std::unexpected(std::format("Kan inte skapa {}: {}", (plan.folder / ".cratchit").string(),
                                       ec ? ec.message() : std::string("mappen finns redan")));
  }
  Project project{plan.folder, plan.folder / ".cratchit" / "project.txt", new_uuid(), plan.name};
  PathValues entries{{"created", std::string(today)}, {"uuid", project.uuid}, {"name", plan.name}};
  if (!plan.orgnr.empty()) entries.push_back({"orgnr", plan.orgnr});
  if (auto written = write_path_values(project.file, entries); !written) return std::unexpected(written.error());
  if (auto written = books::write_books(plan.folder, plan.import.books); !written) return std::unexpected(written.error());
  // The SIE file stays where it is; the inventory tells where the books come from
  auto const day = parse_iso_date(today);
  if (!day) return std::unexpected(std::format("{} är inget datum", today));
  if (auto recorded = record_sie_use(plan.folder, plan.sie_file, {"ny bokföring", plan.import.vouchers_added, 0}, *day);
      !recorded) {
    return std::unexpected(std::format("cratchit skapade bokföringen, men kunde inte skriva förteckningen: {}", recorded.error()));
  }
  return project;
}

} // namespace cratchit
