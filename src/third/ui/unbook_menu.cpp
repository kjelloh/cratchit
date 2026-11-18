#include "ui/unbook_menu.hpp"

#include "charset/utf8.hpp"
#include "core/amount.hpp"
#include "core/books/books.hpp"
#include "core/date.hpp"
#include "core/event_draft.hpp"
#include "core/log.hpp"
#include "core/sie_summary.hpp"
#include "ui/help_topics.hpp"
#include "ui/year_view.hpp"

#include <algorithm>
#include <expected>
#include <format>
#include <string>
#include <utility>
#include <vector>

namespace cratchit::ui {

namespace {

using pyroceph::tui::Accepted;
using pyroceph::tui::CodePoint;
using pyroceph::tui::InputResult;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

std::string utf8_of(std::u32string_view text) {
  std::string typed;
  charset::utf8::Encoder encoder;
  for (auto const code_point : text) {
    for (auto const byte : encoder.push(code_point)) typed.push_back(static_cast<char>(byte));
  }
  return typed;
}

class UnbookState : public State {
public:
  UnbookState(Project project, FiscalYear year, State const* origin, std::function<void(std::string)> report)
      : project_(std::move(project)), year_(std::move(year)), origin_(origin), report_(std::move(report)) {
    read();
  }
  std::string name() const override { return "Gör till utkast"; }
  View view() const override {
    if (!error_.empty()) return {std::format("cratchit kunde inte läsa bokföringen: {}", error_)};
    if (last_.empty()) return {std::format("Inget bokfört i {}.", label_of(year_))};
    View lines{std::format("Den sista verifikationen i varje serie i {}:", label_of(year_))};
    for (auto const& voucher : last_) {
      auto line = std::format("  {}  {}", voucher.series, unindented(voucher_row(voucher)));
      if (voucher.registered) line += "  registrerad " + format_iso_date(*voucher.registered);
      lines.push_back(std::move(line));
    }
    lines.push_back("Kom verifikationen från ett annat program med en SIE-fil, eller har du exporterat den dit, finns "
                    "den kvar där. Då skiljer sig bokföringarna tills du har rättat där också.");
    lines.push_back("Skriv seriens bokstav och tryck Enter för att göra verifikationen till ett utkast.");
    return lines;
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  InputResult on_input(std::u32string_view text) override {
    auto series = utf8_of(text);
    std::erase(series, ' ');
    for (auto& c : series) {
      if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    }
    if (series.empty()) return std::unexpected(std::string("Skriv seriens bokstav, till exempel A."));
    auto const found = std::ranges::find(last_, series, &books::Voucher::series);
    if (found == last_.end()) {
      return std::unexpected(std::format("Serie {} har ingen verifikation i {}.", series, label_of(year_)));
    }
    auto const year_start = parse_iso_date(year_.start);
    if (!year_start) return std::unexpected(std::format("Felaktigt räkenskapsår {}", year_.start));
    auto const id = books::voucher_id(found->series, found->number);
    auto const index = unbook_voucher(project_.folder, *year_start, *found, today_date());
    if (!index) {
      LOG_BUSINESS("Kunde inte göra {} till utkast i {} ({}): {}", id, label_of(year_), project_.folder.string(),
                   index.error());
      return std::unexpected(index.error());
    }
    auto const draft = draft_id(*index, draft_count(project_.folder, *year_start));
    LOG_BUSINESS("Verifikation {} gjord till utkast {} i {} ({}): {} {} \"{}\", registrerad {}, signatur \"{}\"", id,
                 draft, label_of(year_), project_.folder.string(), format_iso_date(found->event.date),
                 format_amount(found->event.amount), found->event.heading,
                 found->registered ? format_iso_date(*found->registered) : std::string(), found->signature);
    for (auto const& posting : found->postings) {
      LOG_BUSINESS("  kontering: {} {} {} \"{}\"{}", posting.account, format_amount(posting.amount),
                   posting.date ? format_iso_date(*posting.date) : std::string(), posting.text,
                   posting.kind == books::Posting::Kind::removed ? " (struken)" : "");
    }
    report_(std::format("cratchit gjorde {} till utkast {}: {}.", id, draft, found->event.heading));
    done_ = true;
    return Accepted(); // back to origin
  }
  State const* back_to() const override { return done_ ? origin_ : nullptr; }
  std::string help_topic() const override { return std::string(help_topic::unbook); }

private:
  // A row of voucher_rows without its indent
  static std::string unindented(std::string row) { return row.starts_with("  ") ? row.substr(2) : row; }

  void read() {
    auto const books = books::read_books(project_.folder);
    if (!books) {
      error_ = books.error();
      return;
    }
    auto const start = parse_iso_date(year_.start);
    if (auto const* year = start ? books->year_starting(*start) : nullptr; year != nullptr) last_ = last_vouchers(*year);
  }

  Project project_;
  FiscalYear year_;
  State const* origin_;
  std::function<void(std::string)> report_;
  std::vector<books::Voucher> last_;
  std::string error_;
  bool done_ = false;
};

} // namespace

std::unique_ptr<State> make_unbook_menu(Project project, FiscalYear year, State const* origin,
                                        std::function<void(std::string)> report) {
  return std::make_unique<UnbookState>(std::move(project), std::move(year), origin, std::move(report));
}

} // namespace cratchit::ui
