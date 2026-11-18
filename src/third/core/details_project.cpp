#include "core/details_project.hpp"

#include "core/books/books.hpp"
#include "core/date.hpp"
#include "core/path_value.hpp"
#include "core/sie_project.hpp"

#include <charconv>
#include <chrono>
#include <format>
#include <system_error>

namespace cratchit {

namespace fs = std::filesystem;

namespace {

std::string_view trimmed(std::string_view text) {
  auto const first = text.find_first_not_of(" \t");
  if (first == std::string_view::npos) return {};
  return text.substr(first, text.find_last_not_of(" \t") - first + 1);
}

bool luhn_ok(std::string_view digits) {
  int sum = 0;
  for (std::size_t i = 0; i < digits.size(); ++i) {
    int value = digits[i] - '0';
    if (i % 2 == 0) value *= 2; // from the left: 2, 1, 2, 1, ... over ten digits
    sum += value > 9 ? value - 9 : value;
  }
  return sum % 10 == 0;
}

} // namespace

std::expected<fs::path, std::string> new_project_folder(Workspace const& workspace, std::string_view name) {
  name = trimmed(name);
  if (name.empty()) return std::unexpected("Skriv företagets namn");
  if (!is_folder_name(name)) {
    return std::unexpected(std::format("Namnet \"{}\" går inte att använda som namn på en mapp", name));
  }
  auto const folder = workspace.folder / fs::path(std::u8string(name.begin(), name.end()));
  std::error_code ec;
  if (fs::exists(folder, ec)) return std::unexpected(std::format("Mappen {} finns redan", folder.string()));
  return folder;
}

std::expected<std::string, std::string> parse_orgnr(std::string_view text) {
  text = trimmed(text);
  std::string digits;
  for (std::size_t i = 0; i < text.size(); ++i) {
    auto const c = text[i];
    if (c >= '0' && c <= '9') {
      digits.push_back(c);
    } else if (!(c == '-' && i == 6)) {
      return std::unexpected(std::format("\"{}\" är inget organisationsnummer. Skriv tio siffror, till exempel 556677-8899", text));
    }
  }
  if (digits.size() != 10) {
    return std::unexpected(std::format("\"{}\" är inget organisationsnummer. Skriv tio siffror, till exempel 556677-8899", text));
  }
  if (!luhn_ok(digits)) return std::unexpected(std::format("Kontrollsiffran i {} stämmer inte", text));
  return digits.substr(0, 6) + "-" + digits.substr(6);
}

std::expected<std::chrono::year_month, std::string> parse_first_month(std::string_view text) {
  text = trimmed(text);
  auto const date = text.size() == 7 ? parse_iso_date(std::string(text) + "-01") : std::nullopt;
  if (!date) return std::unexpected(std::format("\"{}\" är ingen månad. Skriv år och månad, till exempel 2026-05", text));
  return std::chrono::year_month(date->year(), date->month());
}

std::expected<int, std::string> parse_first_year_length(std::string_view text) {
  text = trimmed(text);
  int months = 0;
  auto const [end, error] = std::from_chars(text.data(), text.data() + text.size(), months);
  if (error != std::errc{} || end != text.data() + text.size() || text.empty()) {
    return std::unexpected(std::format("\"{}\" är inget antal månader. Skriv till exempel 12", text));
  }
  if (months < 1 || months > 18) return std::unexpected("Det första räkenskapsåret får vara 1 till 18 månader");
  return months;
}

FiscalYear first_fiscal_year(std::chrono::year_month first_month, int months) {
  using namespace std::chrono;
  auto const start = year_month_day(first_month / day{1});
  auto const last_month = first_month + std::chrono::months(months - 1);
  auto const end = year_month_day(last_month / last);
  return {format_iso_date(start), format_iso_date(end)};
}

namespace {

// True when a and b are the same folder on disk (links resolved); false when either is missing
bool same_folder(fs::path const& a, fs::path const& b) {
  std::error_code ec;
  return fs::equivalent(a, b, ec) && !ec;
}

} // namespace

std::expected<void, std::string> check_project_folder(fs::path const& folder, std::optional<fs::path> const& workspace_folder,
                                                      std::optional<fs::path> const& home, std::string_view subject,
                                                      std::string_view remedy) {
  std::error_code ec;
  if (home && same_folder(folder, *home)) return std::unexpected(std::format("{} är din hemkatalog. {}", subject, remedy));
  if (workspace_folder && same_folder(folder, *workspace_folder)) {
    return std::unexpected(std::format("{} är samlingsmappen. {}", subject, remedy));
  }
  if (fs::exists(folder / ".cratchit", ec)) {
    return std::unexpected(std::format("{} har redan en .cratchit-mapp: {}", subject, (folder / ".cratchit").string()));
  }
  for (auto above = folder.parent_path(); !above.empty() && above != above.parent_path(); above = above.parent_path()) {
    if (fs::is_regular_file(above / ".cratchit" / "project.txt", ec)) {
      return std::unexpected(std::format("{} ligger inuti bokföringen i {}.", subject, above.string()));
    }
  }
  return {};
}

std::expected<void, std::string> check_project_here(fs::path const& folder, std::optional<fs::path> const& workspace_folder,
                                                    std::optional<fs::path> const& home) {
  return check_project_folder(folder, workspace_folder, home, "Arbetsmappen", "Starta cratchit i bokföringens egen mapp.");
}

std::expected<Project, std::string> create_project_from_details(fs::path const& folder, std::string_view name,
                                                                std::string_view orgnr, FiscalYear const& year,
                                                                std::string_view today) {
  std::error_code ec;
  if (!fs::create_directory(folder, ec)) {
    return std::unexpected(std::format("Kan inte skapa {}: {}", folder.string(), ec ? ec.message() : std::string("mappen finns redan")));
  }
  return create_project_here(folder, name, orgnr, year, today);
}

std::expected<Project, std::string> create_project_here(fs::path const& folder, std::string_view name,
                                                        std::string_view orgnr, FiscalYear const& year,
                                                        std::string_view today) {
  std::error_code ec;
  if (!fs::create_directory(folder / ".cratchit", ec)) {
    return std::unexpected(std::format("Kan inte skapa {}: {}", (folder / ".cratchit").string(),
                                       ec ? ec.message() : std::string("mappen finns redan")));
  }
  Project project{folder, folder / ".cratchit" / "project.txt", new_uuid(), std::string(name)};
  PathValues const entries{{"created", std::string(today)}, {"uuid", project.uuid}, {"name", project.name},
                           {"orgnr", std::string(orgnr)}};
  if (auto written = write_path_values(project.file, entries); !written) return std::unexpected(written.error());
  // Only the fiscal year: the tables of the books come when there is something to put in them
  auto const books_folder = books::books_folder(folder);
  if (!fs::create_directory(books_folder, ec)) {
    return std::unexpected(std::format("Kan inte skapa {}: {}", books_folder.string(), ec.message()));
  }
  if (auto written = write_path_values(books_folder / "books.txt", {{"year." + year.start, year.end}}); !written) {
    return std::unexpected(written.error());
  }
  return project;
}

} // namespace cratchit
