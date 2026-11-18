#include "core/skv/vat_return/report.hpp"

#include "core/skv/vat_return/box_texts.hpp"
#include "core/skv/vat_return/review.hpp"

#include <algorithm>
#include <format>

namespace cratchit::skv::vat_return {

namespace {

// Box, account, voucher, date, amount, text
std::string row(std::string_view box, std::string_view account, std::string_view voucher, std::string_view date,
                std::string_view amount, std::string_view text) {
  auto line = std::format("{:<5}{:<9}{:<9}{:<12}{:>15}", box, account, voucher, date, amount);
  if (!text.empty()) {
    line += "  ";
    line += text;
  }
  return line;
}

// A box title longer than this many characters is cut, so the report's lines stay readable in a terminal
constexpr std::size_t max_title = 45;

// The text of the voucher a posting is in, if the books have it
std::string heading_of(books::Books const& books, VoucherPosting const& posting) {
  auto const* year = books.year_starting(posting.fiscal_year);
  if (!year) {
    return {};
  }
  auto const it = std::ranges::find_if(year->vouchers, [&](books::Voucher const& voucher) {
    return voucher.series == posting.series && voucher.number == posting.number;
  });
  return it == year->vouchers.end() ? std::string{} : it->event.heading;
}

// A voucher of a step with its date and amount
std::string step_line(std::string_view step, VoucherPosting const& voucher) {
  return std::format("  {:<10}{:>15}  {}  {}", step, format_amount(voucher.amount),
                     books::voucher_id(voucher.series, voucher.number), format_iso_date(voucher.date));
}

} // namespace

std::string vouchers_text(std::vector<VoucherPosting> const& vouchers) {
  std::string text;
  for (auto const& voucher : vouchers) {
    text += text.empty() ? "" : ", ";
    text += books::voucher_id(voucher.series, voucher.number);
  }
  return text;
}

std::string steps_heading() {
  return std::format("{:>12}{:>12}{:>12}", "Period", "Cleared", "Charged");
}

std::string steps_row(PeriodSteps const& steps) {
  auto line = std::format("{:>12}{:>12}{:>12}", format_amount(steps.period_vat), format_amount(steps.cleared),
                          format_amount(steps.charged));
  if (!steps.clearings.empty()) {
    line += "  cleared by " + vouchers_text(steps.clearings);
  }
  if (!steps.charges.empty()) {
    line += ", charged by " + vouchers_text(steps.charges);
  }
  return line;
}

std::string quarter_name(Period period) {
  return std::format("{} Q{}", static_cast<int>(period.last.year()), (static_cast<unsigned>(period.last.month()) + 2) / 3);
}

std::vector<std::string> report_of(books::Books const& books, Document const& document, PeriodSteps const& steps,
                                   std::string_view company, std::optional<Date> so_far) {
  std::vector<std::string> lines{std::format("Momsrapport {} – {}{}", format_iso_date(document.period.first),
                                             format_iso_date(document.period.last),
                                             so_far ? std::format(", hittills (i dag {})", format_iso_date(*so_far))
                                                    : std::string{}),
                                 std::string(company), ""};
  auto const add_steps = [&] {
    lines.push_back("");
    lines.push_back("Periodens moms:");
    lines.push_back(std::format("  {:<10}{:>15}", "Perioden", format_amount(steps.period_vat)));
    if (steps.clearings.empty()) {
      lines.push_back(std::format("  {:<10}{:>15}", "Avräknad", format_amount(0)));
    }
    for (auto const& clearing : steps.clearings) {
      lines.push_back(step_line("Avräknad", clearing));
    }
    if (steps.charges.empty()) {
      lines.push_back(std::format("  {:<10}{:>15}", "Debiterad", format_amount(0)));
    }
    for (auto const& charge : steps.charges) {
      lines.push_back(step_line("Debiterad", charge));
    }
  };
  if (document.boxes.empty()) {
    lines.push_back("Inga konteringar i perioden på konton som redovisas i en ruta.");
    add_steps();
    return lines;
  }
  lines.push_back(row("Ruta", "Konto", "Ver", "Datum", "Belopp", "Text"));
  // In the form's order, each section of the form headed where it starts, each box with its title
  auto boxes = document.boxes;
  std::ranges::stable_sort(boxes, {}, [](BoxAmount const& box) { return form_position(box.box); });
  std::string section;
  auto const head = [&](Box const& box) {
    if (auto const text = text_of(box); text && text->section != section) {
      section = text->section;
      lines.push_back(section);
    }
  };
  for (auto const& box : boxes) {
    lines.push_back("");
    head(box.box);
    auto const text = text_of(box.box, box.amount);
    lines.push_back(row(box.box, "", "", "", format_amount(box.amount), text ? cut_text(text->title, max_title) : ""));
    for (auto const& account : box.accounts) {
      auto const* known = books.account(account.account);
      lines.push_back(row("", account.account, "", "", format_amount(account.amount), known ? known->name : ""));
      for (auto const& posting : account.postings) {
        lines.push_back(row("", "", books::voucher_id(posting.series, posting.number), format_iso_date(posting.date),
                            format_amount(posting.amount), heading_of(books, posting)));
      }
    }
  }
  auto const to_pay = vat_to_pay(document);
  lines.push_back("");
  head(Box(vat_to_pay_box));
  lines.push_back(row(vat_to_pay_box, "", "", "", format_amount(to_pay), vat_to_pay_expression));
  add_steps();
  lines.push_back("");
  lines.push_back("Rutornas belopp är som på blanketten; konton och verifikationer som i bokföringen: debet "
                  "positivt, kredit negativt.");
  lines.push_back(to_pay > 0   ? std::format("Ruta 49 på blanketten: {} att betala.", format_amount(to_pay))
                  : to_pay < 0 ? std::format("Ruta 49 på blanketten: {} att få tillbaka.", format_amount(-to_pay))
                               : std::string("Ruta 49 på blanketten: 0.00, inget att betala eller få tillbaka."));
  return lines;
}

} // namespace cratchit::skv::vat_return
