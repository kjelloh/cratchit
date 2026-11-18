#include "ui/booked_menu.hpp"

#include "charset/utf8.hpp"
#include "core/books/books.hpp"
#include "core/amount.hpp"
#include "core/date.hpp"
#include "core/event_draft.hpp"
#include "core/sie_summary.hpp"
#include "core/log.hpp"
#include "pyroceph/tui/pager.hpp"
#include "ui/help_topics.hpp"
#include "ui/reports_menu.hpp"
#include "ui/unbook_menu.hpp"
#include "ui/voucher_menu.hpp"
#include "ui/year_view.hpp"

#include <algorithm>
#include <expected>
#include <format>
#include <string>
#include <utility>

namespace cratchit::ui {

namespace {

using pyroceph::tui::Accepted;
using pyroceph::tui::CodePoint;
using pyroceph::tui::InputResult;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

constexpr CodePoint back_key = U'<';

// Voucher numbers compare as numbers, so that D1 is never D11 (nor D01 another than D1)
bool same_number(std::string const& a, std::string const& b) {
  auto const strip = [](std::string const& n) {
    auto const first = n.find_first_not_of('0');
    return first == std::string::npos ? std::string("0") : n.substr(first);
  };
  return strip(a) == strip(b);
}
constexpr CodePoint forward_key = U'>';

// Voucher numbers as numbers, for comparing (an empty or odd number counts as 0)
unsigned long number_of(std::string const& number) {
  unsigned long value = 0;
  for (auto const c : number) {
    if (c < '0' || c > '9') return 0;
    value = value * 10 + static_cast<unsigned long>(c - '0');
  }
  return value;
}

// The id of the last voucher in series among vouchers (the highest number)
std::string last_in_series(std::vector<books::Voucher> const& vouchers, std::string const& series) {
  books::Voucher const* last = nullptr;
  for (auto const& voucher : vouchers) {
    if (voucher.series == series && (last == nullptr || number_of(voucher.number) > number_of(last->number))) last = &voucher;
  }
  return last != nullptr ? books::voucher_id(last->series, last->number) : std::string();
}

class BookedState : public State {
public:
  BookedState(Project project, FiscalYear year) : project_(std::move(project)), year_(std::move(year)) { read(); }
  std::string name() const override { return "Bokfört"; }
  View view() const override {
    if (!error_.empty()) return {std::format("cratchit kunde inte läsa bokföringen: {}", error_)};
    if (pager_.count() == 0) return {std::format("Bokfört {}: inget bokfört i räkenskapsåret.", label_of(year_))};
    View lines{std::format("Bokfört {}: verifikationer {}–{} av {}", label_of(year_), pager_.first() + 1,
                           pager_.first() + pager_.shown(), pager_.count())};
    auto const page = pager_.view();
    lines.insert(lines.end(), page.begin(), page.end());
    lines.insert(lines.end(), report_.begin(), report_.end());
    return lines;
  }
  // Always both, so that turning past either end does nothing (and starts no input)
  OptionsList options() const override {
    return {{U'0', "Gör till utkast"}, {back_key, "Föregående sida"}, {forward_key, "Nästa sida"}};
  }
  // < and > turn the page in place (keys match exactly, beslut.md)
  bool on_key_in_place(CodePoint key) override {
    if (key == back_key) return pager_.back();
    if (key == forward_key) return pager_.forward();
    return false;
  }
  OnKeyResult on_key(CodePoint key) override {
    if (key != U'0') return nullptr;
    report_.clear();
    return make_unbook_menu(project_, year_, this, [this](std::string line) { report_.push_back(std::move(line)); });
  }
  // The id of a voucher as one word, e.g. A12, opens it
  InputResult on_input(std::u32string_view text) override {
    std::string typed;
    charset::utf8::Encoder encoder;
    for (auto const code_point : text) {
      for (auto const byte : encoder.push(code_point)) typed.push_back(static_cast<char>(byte));
    }
    auto const id = books::parse_voucher_id(typed);
    if (!id) return std::unexpected(std::string("Skriv verifikationens id som ett ord, till exempel A12."));
    auto const books = books::read_books(project_.folder);
    if (!books) return std::unexpected(std::format("cratchit kunde inte läsa bokföringen: {}", books.error()));
    auto const start = parse_iso_date(year_.start);
    auto const* year = start ? books->year_starting(*start) : nullptr;
    if (year != nullptr) {
      auto const& [series, number] = *id;
      auto const found = std::ranges::find_if(year->vouchers, [&](books::Voucher const& v) {
        return v.series == series && same_number(v.number, number);
      });
      if (found != year->vouchers.end()) {
        auto reverse = [project = project_, year = year_, voucher = *found]() -> std::expected<View, std::string> {
          auto const id = books::voucher_id(voucher.series, voucher.number);
          auto const year_start = parse_iso_date(year.start);
          if (!year_start) return std::unexpected(std::format("Felaktigt räkenskapsår {}", year.start));
          auto const index = add_reversal_draft(project.folder, *year_start, voucher, today_date());
          if (!index) {
            LOG_BUSINESS("Kunde inte skapa ändringsverifikation för {} i {} ({}): {}", id, label_of(year),
                         project.folder.string(), index.error());
            return std::unexpected(index.error());
          }
          auto const draft = draft_id(*index, draft_count(project.folder, *year_start));
          LOG_BUSINESS("Ändringsverifikation för {} skapad som utkast {} i {} ({}): {} {}", id, draft,
                       label_of(year), project.folder.string(), format_iso_date(voucher.event.date),
                       format_amount(-voucher.event.amount));
          return View{std::format("cratchit skapade utkast {}: Ändring av {}, daterat {} som {}.", draft, id,
                                  format_iso_date(voucher.event.date), id),
                      "Se till att datumet inte ligger i ett år eller en momsperiod som redan är rapporterad; då krävs "
                      "en ny rapport. Datumet ändrar du med 0 i utkastets vy innan du bokför."};
        };
        auto print = [project = project_, year = year_, voucher = *found] { return make_voucher_printout(project, year, voucher); };
        return Accepted(make_voucher_menu(*found, books->accounts, books->series_name(found->series),
                                          last_in_series(year->vouchers, found->series), std::move(reverse),
                                          std::move(print)));
      }
    }
    return std::unexpected(std::format("Det finns ingen verifikation {} i {}.", typed, label_of(year_)));
  }
  void on_return() override { read(); }
  std::string help_topic() const override { return std::string(help_topic::booked); }

private:
  void read() {
    error_.clear();
    auto const books = books::read_books(project_.folder);
    auto const start = parse_iso_date(year_.start);
    if (!books) {
      LOG_BUSINESS("Kunde inte läsa bokföringen i {}: {}", project_.folder.string(), books.error());
      error_ = books.error();
      pager_.set({});
      return;
    }
    pager_.set(voucher_rows(start ? books->year_starting(*start) : nullptr));
  }

  Project project_;
  FiscalYear year_;
  pyroceph::tui::Pager pager_{booked_page_rows};
  std::string error_;
  View report_; // what was just done (Gör till utkast), until the user goes on
};

} // namespace

std::unique_ptr<State> make_booked_menu(Project project, FiscalYear year) {
  return std::make_unique<BookedState>(std::move(project), std::move(year));
}

} // namespace cratchit::ui
