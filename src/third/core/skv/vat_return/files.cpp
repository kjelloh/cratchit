#include "core/skv/vat_return/files.hpp"

#include "core/amount.hpp"
#include "core/skv/vat_return/checks.hpp"
#include "core/skv/vat_return/report.hpp"

#include <format>
#include <fstream>

namespace cratchit::skv::vat_return {

std::string eskd_period(Period const& period) {
  return std::format("{:04}{:02}", static_cast<int>(period.last.year()), static_cast<unsigned>(period.last.month()));
}

std::vector<std::string> report_with_checks(books::Books const& books, Document const& document,
                                            PeriodSteps const& steps, std::string_view company,
                                            std::optional<Date> so_far, std::string_view made_by) {
  auto report = report_of(books, document, steps, company, so_far);
  auto const checks = checks_of(books, document.period);
  report.push_back("");
  report.push_back("Kontroller:");
  if (checks.empty()) report.push_back("  Kontrollerna hittade inget att titta på.");
  for (auto const& check : checks) report.push_back("  " + check);
  report.push_back("");
  report.push_back(std::string(made_by));
  return report;
}

std::vector<std::string> voucher_lines(books::Books const& books, std::optional<Clearing> const& clearing,
                                       PeriodSteps const& steps) {
  auto const& period = steps.period;
  if (!steps.clearings.empty()) {
    return {std::format("Momsavräkning {} – {}", format_iso_date(period.first), format_iso_date(period.last)), "",
            std::format("Perioden är redan avräknad av {} i bokföringen. Ingen ny verifikation behövs.",
                        vouchers_text(steps.clearings))};
  }
  std::vector<std::string> lines{
      std::format("Förslag till momsavräkning {} – {}", format_iso_date(period.first), format_iso_date(period.last)),
      ""};
  if (!clearing) {
    lines.push_back("Inget momskonto ändrades i perioden: det finns inget att avräkna.");
    return lines;
  }
  lines.push_back(std::format("Serie:   {} (inte bokförd: bokför den själv)", clearing_series));
  lines.push_back(std::format("Datum:   {}", format_iso_date(clearing->event.date)));
  lines.push_back(std::format("Rubrik:  {}", clearing->event.heading));
  lines.push_back(std::format("Belopp:  {}", format_amount(clearing->event.amount)));
  lines.push_back("");
  lines.push_back(std::format("{:<8}{:>15}{:>15}  {}", "Konto", "Debet", "Kredit", "Kontonamn"));
  for (auto const& posting : clearing->postings) {
    auto const* account = books.account(posting.account);
    auto const debit = posting.amount > 0 ? format_amount(posting.amount) : std::string();
    auto const credit = posting.amount < 0 ? format_amount(-posting.amount) : std::string();
    lines.push_back(std::format("{:<8}{:>15}{:>15}  {}", posting.account, debit, credit, account ? account->name : ""));
  }
  lines.push_back(std::format("{:<8}{:>15}{:>15}", "Summa", format_amount(books::debit(clearing->postings)),
                              format_amount(books::credit(clearing->postings))));
  return lines;
}

std::expected<void, std::string> write_text(std::filesystem::path const& file, std::vector<std::string> const& lines) {
  std::ofstream out(file, std::ios::binary);
  for (auto const& line : lines) out << line << '\n';
  out.close();
  if (!out) return std::unexpected(std::format("Cannot write {}", file.string()));
  return {};
}

} // namespace cratchit::skv::vat_return
