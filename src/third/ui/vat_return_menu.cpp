#include "ui/vat_return_menu.hpp"

#include "charset/utf8.hpp"
#include "core/amount.hpp"
#include "core/books/books.hpp"
#include "core/date.hpp"
#include "core/event_draft.hpp"
#include "core/log.hpp"
#include "core/sie_summary.hpp"
#include "core/skv/vat_return/checks.hpp"
#include "core/skv/vat_return/clearing.hpp"
#include "core/skv/vat_return/document.hpp"
#include "core/skv/vat_return/eskd.hpp"
#include "core/skv/vat_return/files.hpp"
#include "core/skv/vat_return/report.hpp"
#include "core/skv/vat_return/review.hpp"
#include "core/skv/vat_return/steps.hpp"
#include "core/version.hpp"
#include "pyroceph/tui/pager.hpp"
#include "ui/draft_menu.hpp"
#include "ui/help_topics.hpp"
#include "ui/year_view.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace cratchit::ui {

namespace {

namespace fs = std::filesystem;
namespace vat = skv::vat_return;
using pyroceph::tui::CodePoint;
using pyroceph::tui::InputResult;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

constexpr CodePoint page_back = U'<';
constexpr CodePoint page_forward = U'>';
constexpr CodePoint running_key = U'+';

std::string utf8_of(std::u32string_view text) {
  std::string typed;
  charset::utf8::Encoder encoder;
  for (auto const code_point : text) {
    for (auto const byte : encoder.push(code_point)) typed.push_back(static_cast<char>(byte));
  }
  return typed;
}

// The width of UTF-8 text in the terminal, counted in code points
std::size_t width_of(std::string_view text) {
  return static_cast<std::size_t>(
      std::ranges::count_if(text, [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; }));
}

// The books as Momsrapport sees them, read anew each time
struct VatBooks {
  books::Books books;
  vat::Steps steps;    // every quarter of the books, and today's
  std::string orgnr;   // from project.txt, may be empty
  std::string company; // "The ITfied AB, 556677-8899"
};

// Every quarter ending in a fiscal year of the books, and the one running today,
// oldest first, so that a charge finds its quarter (as take 3)
std::vector<vat::Period> all_quarters(books::Books const& books, Date today) {
  std::vector<vat::Period> periods;
  for (auto const& year : books.years) {
    for (auto const& period : vat::quarters_ending_in(year.start, year.end)) {
      if (std::ranges::find(periods, period) == periods.end()) periods.push_back(period);
    }
  }
  auto const running = vat::quarter_holding(today);
  if (std::ranges::find(periods, running) == periods.end() && !books.years.empty() &&
      running.first > books.years.back().end) {
    periods.push_back(running);
  }
  std::ranges::sort(periods, {}, &vat::Period::first);
  return periods;
}

std::expected<VatBooks, std::string> read_vat_books(Project const& project, Date today) {
  auto books = books::read_books(project.folder);
  if (!books) return std::unexpected(books.error());
  VatBooks read;
  read.steps = vat::steps_of(*books, all_quarters(*books, today));
  read.books = std::move(*books);
  read.orgnr = describe_project(ListedProject{project.uuid, project.folder}).orgnr;
  read.company = read.orgnr.empty() ? project.name : std::format("{}, {}", project.name, read.orgnr);
  return read;
}

vat::PeriodSteps steps_of_period(vat::Steps const& steps, vat::Period const& period) {
  auto const found = std::ranges::find(steps.periods, period, &vat::PeriodSteps::period);
  if (found != steps.periods.end()) return *found;
  vat::PeriodSteps none;
  none.period = period;
  return none;
}

// The three steps in columns, then the vouchers that cleared and charged the period
std::string steps_text(vat::PeriodSteps const& steps) {
  auto line = std::format("{:>12}{:>12}{:>12}", format_amount(steps.period_vat), format_amount(steps.cleared),
                          format_amount(steps.charged));
  if (!steps.clearings.empty()) line += "  avräknat av " + vat::vouchers_text(steps.clearings);
  if (!steps.charges.empty()) line += ", debiterat av " + vat::vouchers_text(steps.charges);
  return line;
}

std::string quarter_caption(vat::Period const& period) { return std::format("{:<9}", vat::quarter_name(period)); }

// The VAT return's folder for the period in the project's folder, e.g. moms_2026-07-01_2026-09-30
std::string folder_name(vat::Period const& period) {
  return std::format("moms_{}_{}", format_iso_date(period.first), format_iso_date(period.last));
}

std::string utf8_name(fs::path const& path) {
  auto const name = path.filename().u8string();
  return {name.begin(), name.end()};
}

// Granska rutorna and a box: the quarter's return as it was read
struct Reviewed {
  books::Books books;
  vat::Document document;
  std::vector<vat::ReviewBox> boxes;
  std::string quarter; // e.g. "2026 Q3"
};

// Ruta: one box against SKV's description; < and > the box before and after
class VatBoxState : public State {
public:
  VatBoxState(std::shared_ptr<Reviewed const> reviewed, std::size_t index)
      : reviewed_(std::move(reviewed)), index_(index) {}
  std::string name() const override { return "Ruta " + reviewed_->boxes[index_].box; }
  View view() const override {
    return vat::review_of(reviewed_->books, reviewed_->document, reviewed_->boxes[index_].box);
  }
  OptionsList options() const override {
    OptionsList options;
    if (index_ > 0) options.push_back({page_back, "Föregående ruta"});
    if (index_ + 1 < reviewed_->boxes.size()) options.push_back({page_forward, "Nästa ruta"});
    return options;
  }
  bool on_key_in_place(CodePoint key) override {
    if (key == page_back && index_ > 0) {
      --index_;
      return true;
    }
    if (key == page_forward && index_ + 1 < reviewed_->boxes.size()) {
      ++index_;
      return true;
    }
    return false;
  }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::string help_topic() const override { return std::string(help_topic::vat_box); }

private:
  std::shared_ptr<Reviewed const> reviewed_;
  std::size_t index_;
};

// Granska rutorna: every box of the form under its section, its title (cut) and
// how many postings it has; a box number and Enter shows it (5 is box 05)
class VatReviewState : public State {
public:
  explicit VatReviewState(std::shared_ptr<Reviewed const> reviewed) : reviewed_(std::move(reviewed)) {}
  std::string name() const override { return "Granska rutorna"; }
  View view() const override {
    View lines{std::format("Rutorna i {} att granska mot Skatteverkets beskrivningar:", reviewed_->quarter)};
    std::string section;
    for (auto const& box : reviewed_->boxes) {
      if (box.section != section) {
        section = box.section;
        lines.push_back(section);
      }
      auto title = vat::cut_text(box.title, 45);
      for (auto shown = width_of(title); shown < 45; ++shown) title += ' ';
      lines.push_back(std::format("  {}  {}  {:>16}", box.box, title,
                                  box.postings == 0   ? std::string("inga konteringar")
                                  : box.postings == 1 ? std::string("1 kontering")
                                                      : std::format("{} konteringar", box.postings)));
    }
    lines.push_back("Skriv rutans nummer och tryck Enter.");
    return lines;
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  InputResult on_input(std::u32string_view text) override {
    auto typed = utf8_of(text);
    std::erase(typed, ' ');
    if (typed.size() == 1) typed = "0" + typed;
    auto const found = std::ranges::find(reviewed_->boxes, typed, &vat::ReviewBox::box);
    if (found == reviewed_->boxes.end()) {
      return std::unexpected(std::format("Det finns ingen ruta {} att granska. Skriv en ruta i listan, till exempel 48.",
                                         utf8_of(text)));
    }
    return pyroceph::tui::Accepted(std::make_unique<VatBoxState>(
        reviewed_, static_cast<std::size_t>(found - reviewed_->boxes.begin())));
  }
  std::string help_topic() const override { return std::string(help_topic::vat_review); }

private:
  std::shared_ptr<Reviewed const> reviewed_;
};

// A quarter's VAT return (see make_vat_return_menu)
class VatQuarterState : public State {
public:
  VatQuarterState(Project project, FiscalYear year, vat::Period period)
      : project_(std::move(project)), year_(std::move(year)), period_(period) {
    read();
  }
  std::string name() const override { return vat::quarter_name(period_); }
  View view() const override {
    if (!error_.empty()) return {std::format("cratchit kunde inte läsa bokföringen: {}", error_)};
    auto lines = pager_.view();
    lines.push_back("");
    auto const name = vat::quarter_name(period_);
    if (checks_.empty()) {
      lines.push_back(std::format("Kontroller av {}: inget att titta på.", name));
    } else {
      lines.push_back(std::format("Kontroller av {} (de varnar bara):", name));
      for (auto const& check : checks_) lines.push_back("  ! " + check);
    }
    lines.push_back("");
    if (running()) {
      lines.push_back(std::format("{} pågår till och med {}: momstömning och eSKD-fil när kvartalet har tagit slut.",
                                  name, format_iso_date(period_.last)));
    } else {
      if (!steps_.clearings.empty()) {
        lines.push_back(std::format("{} är redan avräknat av {}.", name, vat::vouchers_text(steps_.clearings)));
      } else if (!clearing_) {
        lines.push_back(std::format("Inget att avräkna: momskontona ändrades inte i {}.", name));
      } else {
        auto const& clearing = *clearing_;
        lines.push_back(std::format("Momstömning (förslag): {} {} \"{}\"", vat::clearing_series,
                                    format_iso_date(clearing.event.date), clearing.event.heading));
        for (auto const& posting : clearing.postings) {
          auto const* account = vat_->books.account(posting.account);
          lines.push_back(std::format("  {}  {:<36}{:>12}", posting.account, account ? account->name : "",
                                      format_amount(posting.amount)));
        }
        if (draft_index_) lines.push_back(std::format("Den finns som utkast {}.", *draft_index_));
      }
      lines.push_back("");
      if (vat_->orgnr.empty()) {
        lines.push_back("eSKD-filen behöver organisationsnumret, som bokföringen saknar (orgnr i .cratchit/project.txt).");
      } else {
        auto const eskd = vat::eskd::document_of(document_, vat_->orgnr);
        lines.push_back(std::format("eSKD-fil {}:",
                                    utf8_name(vat::eskd::free_file(project_.folder / folder_name(period_), eskd.period))));
        for (auto const& field : eskd.fields) {
          lines.push_back(std::format("  {:<24}{:>10}", field.tag, field.amount / 100));
        }
      }
    }
    lines.insert(lines.end(), done_.begin(), done_.end());
    return lines;
  }
  OptionsList options() const override {
    if (!error_.empty()) return {};
    OptionsList options;
    if (pager_.can_back()) options.push_back({page_back, "Föregående sida"});
    if (pager_.can_forward()) options.push_back({page_forward, "Nästa sida"});
    options.push_back({U'0', "Granska rutorna"});
    if (can_clear()) options.push_back({U'1', "Momstömning"});
    options.push_back({U'2', running() ? "Skriv rapport" : "Skriv rapport och eSKD-fil"});
    return options;
  }
  bool on_key_in_place(CodePoint key) override {
    if (!error_.empty()) return false;
    if (key == page_back) return pager_.back();
    if (key == page_forward) return pager_.forward();
    if (key == U'2') {
      write_files();
      return true;
    }
    return false;
  }
  OnKeyResult on_key(CodePoint key) override {
    if (!error_.empty()) return nullptr;
    if (key == U'0') {
      auto reviewed = std::make_shared<Reviewed>(
          Reviewed{vat_->books, document_, vat::review_boxes(document_), vat::quarter_name(period_)});
      return std::make_unique<VatReviewState>(std::move(reviewed));
    }
    if (key == U'1' && can_clear()) return clear();
    return nullptr;
  }
  // Back from Bokför or Granska rutorna: the books as they are now
  void on_return() override { read(); }
  std::string help_topic() const override { return std::string(help_topic::vat_quarter); }

private:
  bool running() const { return period_.last >= today_; }
  bool can_clear() const { return !running() && clearing_ && steps_.clearings.empty(); }

  void read() {
    error_.clear();
    today_ = today_date();
    auto read = read_vat_books(project_, today_);
    if (!read) {
      LOG_BUSINESS("Kunde inte läsa bokföringen i {}: {}", project_.folder.string(), read.error());
      error_ = read.error();
      return;
    }
    vat_ = std::make_shared<VatBooks>(std::move(*read));
    steps_ = steps_of_period(vat_->steps, period_);
    document_ = vat::document_of(vat_->books, period_, vat::Clearings::left_out);
    clearing_ = vat::clearing_of(document_);
    checks_ = vat::checks_of(vat_->books, period_);
    pager_.set(vat::report_of(vat_->books, document_, steps_, vat_->company,
                              running() ? std::optional(today_) : std::nullopt));
    draft_index_.reset();
    if (clearing_) {
      auto const start = parse_iso_date(year_.start);
      auto const* year = start ? vat_->books.year_starting(*start) : nullptr;
      if (year != nullptr) {
        for (std::size_t i = 0; i < year->drafts.size(); ++i) {
          auto const& draft = year->drafts[i];
          if (draft.event.heading == clearing_->event.heading && draft.event.date == clearing_->event.date) {
            draft_index_ = draft_id(i, year->drafts.size());
          }
        }
      }
    }
  }

  // The clearing as a draft (the one there is, else a new one), then Bokför for it
  std::unique_ptr<State> clear() {
    auto const start = parse_iso_date(year_.start);
    if (!start) return pyroceph::tui::make_error_state(std::format("Felaktigt räkenskapsår {}", year_.start));
    books::Draft wanted{clearing_->event, today_, clearing_->postings, std::string(vat::clearing_series)};
    auto const* year = vat_->books.year_starting(*start);
    if (year == nullptr) {
      return pyroceph::tui::make_error_state(std::format("Räkenskapsåret {} finns inte i bokföringen.", label_of(year_)));
    }
    std::expected<std::size_t, std::string> index = std::unexpected(std::string());
    auto const same = std::ranges::find_if(year->drafts, [&](books::Draft const& d) {
      return d.event.heading == wanted.event.heading && d.event.date == wanted.event.date;
    });
    std::string how;
    if (same == year->drafts.end()) {
      index = add_draft(project_.folder, *start, wanted);
      how = "skapad";
    } else if (same->postings == wanted.postings && same->event.amount == wanted.event.amount) {
      index = static_cast<std::size_t>(same - year->drafts.begin());
      wanted = *same;
      how = "finns redan";
    } else {
      wanted.noted = same->noted;
      index = replace_draft(project_.folder, *start, static_cast<std::size_t>(same - year->drafts.begin()), *same,
                            wanted);
      how = "ersatt med bokföringen som den är nu";
    }
    if (!index) {
      LOG_BUSINESS("Kunde inte göra momstömningen för {} till utkast i {} ({}): {}", vat::quarter_name(period_),
                   label_of(year_), project_.folder.string(), index.error());
      return pyroceph::tui::make_error_state(
          std::format("cratchit kunde inte göra momstömningen till ett utkast: {}", index.error()));
    }
    auto const count = draft_count(project_.folder, *start);
    LOG_BUSINESS("Momstömning för {} som utkast {} i {} ({}), {}: {} {} \"{}\"", vat::quarter_name(period_),
                 draft_id(*index, count), label_of(year_), project_.folder.string(), how,
                 format_iso_date(wanted.event.date), format_amount(wanted.event.amount), wanted.event.heading);
    for (auto const& posting : wanted.postings) {
      LOG_BUSINESS("  kontering: {} {}", posting.account, format_amount(posting.amount));
    }
    return make_book_draft_state(project_, year_, *index, count, wanted, this,
                                 [this](std::string line) { done_ = {std::move(line)}; });
  }

  // momsrapport.txt, and for an ended quarter verifikation.txt and the eSKD file, in the period's folder
  void write_files() {
    done_.clear();
    auto const folder = project_.folder / folder_name(period_);
    std::error_code error;
    fs::create_directories(folder, error);
    if (error) {
      done_.push_back(std::format("cratchit kunde inte skapa mappen {}: {}", folder.string(), error.message()));
      return;
    }
    auto const made_by = std::format("Gjord {} av cratchit {}", format_iso_date(today_), version());
    std::vector<std::string> written;
    auto const report = vat::report_with_checks(vat_->books, document_, steps_, vat_->company,
                                                running() ? std::optional(today_) : std::nullopt, made_by);
    if (auto done = vat::write_text(folder / "momsrapport.txt", report); !done) {
      done_.push_back(std::format("cratchit kunde inte skriva momsrapport.txt: {}", done.error()));
      return;
    }
    written.push_back("momsrapport.txt");
    std::optional<std::string> eskd_name;
    if (!running()) {
      if (auto done = vat::write_text(folder / "verifikation.txt", vat::voucher_lines(vat_->books, clearing_, steps_));
          !done) {
        done_.push_back(std::format("cratchit kunde inte skriva verifikation.txt: {}", done.error()));
        return;
      }
      written.push_back("verifikation.txt");
      if (!vat_->orgnr.empty()) {
        auto const file = vat::eskd::write_file(folder, vat::eskd::document_of(document_, vat_->orgnr));
        if (!file) {
          done_.push_back(std::format("cratchit kunde inte skriva eSKD-filen: {}", file.error()));
          return;
        }
        eskd_name = utf8_name(*file);
        written.push_back(*eskd_name);
      }
    }
    std::string list;
    for (std::size_t i = 0; i < written.size(); ++i) {
      list += i == 0 ? "" : i + 1 == written.size() ? " och " : ", ";
      list += written[i];
    }
    LOG_BUSINESS("Momsrapport för {} skriven i {}: {}", vat::quarter_name(period_), folder.string(), list);
    // Each file with its full path, to find it again, e.g. when uploading the eSKD file
    auto const shown = [&folder](std::string const& name) {
      auto const text = (folder / fs::path(std::u8string(name.begin(), name.end()))).u8string();
      return std::string(text.begin(), text.end());
    };
    done_.push_back("cratchit skrev:");
    for (auto const& name : written) done_.push_back("  " + shown(name));
    if (eskd_name) {
      done_.push_back(std::format("Ladda upp {} i Skatteverkets e-tjänst Lämna momsdeklaration.", *eskd_name));
    } else if (!running()) {
      done_.push_back("Ingen eSKD-fil: bokföringen saknar organisationsnummer.");
    }
  }

  Project project_;
  FiscalYear year_;
  vat::Period period_;
  Date today_{};
  std::string error_;
  std::shared_ptr<VatBooks> vat_;
  vat::PeriodSteps steps_;
  vat::Document document_;
  std::optional<vat::Clearing> clearing_;
  std::vector<std::string> checks_;
  std::optional<std::string> draft_index_; // the clearing's draft, e.g. "#2", when there is one
  pyroceph::tui::Pager pager_{vat_report_page_rows};
  View done_; // what was just done here: files written, the clearing booked
};

// Momsrapport: the quarters (see make_vat_return_menu)
class VatQuartersState : public State {
public:
  VatQuartersState(Project project, FiscalYear year) : project_(std::move(project)), year_(std::move(year)) { read(); }
  std::string name() const override { return "Momsrapport"; }
  View view() const override {
    if (!error_.empty()) return {std::format("cratchit kunde inte läsa bokföringen: {}", error_)};
    View lines{std::format("Momskvartal som slutar i {} (i dag {}):", label_of(year_), format_iso_date(today_))};
    if (ended_.empty() && !running_) {
      lines.push_back(std::format("Inget kvartal i {} har börjat än.", label_of(year_)));
      return lines;
    }
    lines.push_back(std::format("{:<13}{:>12}{:>12}{:>12}", "", "Period", "Avräknat", "Debiterat"));
    for (auto const& charge : unmatched_) {
      lines.push_back(std::format("Debiterat, inte kopplat till något kvartal: {}  {}  {}",
                                  books::voucher_id(charge.series, charge.number), format_iso_date(charge.date),
                                  format_amount(charge.amount)));
    }
    return lines;
  }
  OptionsList options() const override {
    if (!error_.empty()) return {};
    OptionsList options;
    if (running_) {
      options.push_back({running_key, quarter_caption(*running_) +
                                          steps_text(steps_of_period(vat_->steps, *running_)) + "  hittills" +
                                          checks_text(*running_)});
    }
    for (std::size_t i = 0; i < ended_.size() && i < 10; ++i) {
      auto const& period = ended_[i];
      options.push_back({static_cast<CodePoint>(U'0' + i),
                         quarter_caption(period) + steps_text(steps_of_period(vat_->steps, period)) +
                             (period == next_ ? "  nästa att rapportera" : "") + checks_text(period)});
    }
    return options;
  }
  OnKeyResult on_key(CodePoint key) override {
    if (!error_.empty()) return nullptr;
    if (key == running_key && running_) return std::make_unique<VatQuarterState>(project_, year_, *running_);
    if (key >= U'0' && key <= U'9') {
      auto const i = static_cast<std::size_t>(key - U'0');
      if (i < ended_.size()) return std::make_unique<VatQuarterState>(project_, year_, ended_[i]);
    }
    return nullptr;
  }
  void on_return() override { read(); }
  std::string help_topic() const override { return std::string(help_topic::vat_return); }

private:
  // "  2 kontroller" after a quarter whose checks found anything
  std::string checks_text(vat::Period const& period) const {
    auto const found = vat::checks_of(vat_->books, period).size();
    if (found == 0) return {};
    return found == 1 ? std::string("  1 kontroll") : std::format("  {} kontroller", found);
  }

  void read() {
    error_.clear();
    ended_.clear();
    running_.reset();
    unmatched_.clear();
    today_ = today_date();
    auto const start = parse_iso_date(year_.start);
    auto const end = parse_iso_date(year_.end);
    if (!start || !end) {
      error_ = std::format("Felaktigt räkenskapsår {} – {}", year_.start, year_.end);
      return;
    }
    auto read = read_vat_books(project_, today_);
    if (!read) {
      LOG_BUSINESS("Kunde inte läsa bokföringen i {}: {}", project_.folder.string(), read.error());
      error_ = read.error();
      return;
    }
    vat_ = std::make_shared<VatBooks>(std::move(*read));
    auto const quarters = vat::quarters_ending_in(*start, *end);
    for (auto it = quarters.rbegin(); it != quarters.rend(); ++it) {
      if (it->last < today_) {
        ended_.push_back(*it);
      } else if (it->first <= today_) {
        running_ = *it;
      }
    }
    next_ = vat::quarter_holding(Date{std::chrono::sys_days{vat::quarter_holding(today_).first} - std::chrono::days{1}});
    for (auto const& charge : vat_->steps.unmatched_charges) {
      if (*start <= charge.date && charge.date <= *end) unmatched_.push_back(charge);
    }
  }

  Project project_;
  FiscalYear year_;
  Date today_{};
  std::string error_;
  std::shared_ptr<VatBooks> vat_;
  std::vector<vat::Period> ended_; // newest first
  std::optional<vat::Period> running_;
  vat::Period next_{};
  std::vector<vat::VoucherPosting> unmatched_;
};

} // namespace

std::unique_ptr<State> make_vat_return_menu(Project project, FiscalYear year) {
  return std::make_unique<VatQuartersState>(std::move(project), std::move(year));
}

} // namespace cratchit::ui
