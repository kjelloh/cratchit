#include "ui/voucher_menu.hpp"

#include "core/same_event.hpp"

#include "core/amount.hpp"
#include "core/date.hpp"
#include "pyroceph/tui/error_state.hpp"
#include "ui/design_insufficiency.hpp"
#include "ui/help_topics.hpp"
#include "ui/year_view.hpp"

#include <algorithm>
#include <format>
#include <memory>
#include <string>
#include <utility>

namespace cratchit::ui {

namespace {

using pyroceph::tui::CodePoint;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

constexpr CodePoint back_key = U'<';
constexpr CodePoint forward_key = U'>';

// The width of UTF-8 text in the terminal, counted in code points
std::size_t width_of(std::string_view text) {
  return static_cast<std::size_t>(std::ranges::count_if(text, [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; }));
}

std::string padded(std::string const& text, std::size_t width) {
  auto const used = width_of(text);
  return used < width ? text + std::string(width - used, ' ') : text;
}

// A label and its value as a row of a view, the values lined up
std::string row(std::string_view label, std::string const& value) {
  return std::format("  {:<16}{}", std::string(label) + ":", value);
}

// What the voucher states share: the voucher and how to name its accounts. A
// draft is shown as a voucher without series and number, with what only a draft has.
struct Shown {
  books::Voucher voucher;
  std::vector<books::Account> accounts;
  std::string series_name;
  std::string last_in_series; // the id of the last voucher in the series, in the fiscal year

  // A draft: its id ("#3"), fiscal year, stage and the day it was noted
  bool draft = false;
  std::string draft_id{};
  std::string year_label{};
  std::string stage{};
  Date noted{};
  std::string summary{};             // one line about it, for Glöm
  State const* origin = nullptr;     // Utkast, to go back to once forgotten or split
  DraftRemoval removal = DraftRemoval::forget; // what 9 does (removal_of)
  DraftActions actions{};
  ReverseVoucher reverse{}; // a voucher: makes its ändringsverifikation as a draft

  std::string id() const { return draft ? draft_id : books::voucher_id(voucher.series, voucher.number); }

  std::string account_name(std::string const& number) const {
    auto const found = std::ranges::find(accounts, number, &books::Account::number);
    return found != accounts.end() ? found->name : std::string();
  }

  // How the voucher is booked: series, registered, signature; for a draft when it was noted
  View booking_rows() const {
    if (draft) {
      View lines{row("Noterad", format_iso_date(noted))};
      if (!voucher.series.empty()) lines.push_back(row("Serie", voucher.series));
      return lines;
    }
    View lines;
    lines.push_back(row("Serie", series_name.empty() ? voucher.series : std::format("{}  {}", voucher.series, series_name)));
    if (voucher.registered) lines.push_back(row("Registrerad", format_iso_date(*voucher.registered)));
    if (!voucher.signature.empty()) lines.push_back(row("Signatur", voucher.signature));
    return lines;
  }

  View event_rows() const {
    // A draft may lack its heading and amount: they show empty
    auto const amount = draft && voucher.event.amount == 0 ? std::string() : format_amount(voucher.event.amount);
    return {row("Datum", format_iso_date(voucher.event.date)), row("Rubrik", voucher.event.heading),
            row("Belopp", amount)};
  }

  // The postings, one line each without indent, the columns lined up over all of them
  std::vector<std::string> posting_lines() const {
    std::size_t account_width = 0;
    std::size_t name_width = 0;
    for (auto const& posting : voucher.postings) {
      account_width = std::max(account_width, width_of(posting.account));
      name_width = std::max(name_width, width_of(account_name(posting.account)));
    }
    std::vector<std::string> lines;
    for (auto const& posting : voucher.postings) {
      auto line = std::format("{}  {}  {:>12}", padded(posting.account, account_width),
                              padded(account_name(posting.account), name_width), format_amount(posting.amount));
      if (!posting.text.empty()) line += "  " + posting.text;
      if (posting.date && *posting.date != voucher.event.date) line += "  " + format_iso_date(*posting.date);
      if (posting.kind == books::Posting::Kind::removed) line += "  (struken)";
      if (posting.kind == books::Posting::Kind::added) line += "  (tillagd)";
      lines.push_back(std::move(line));
    }
    return lines;
  }
};

// One posting of the voucher, named by its number in "Konteringar"
class PostingState : public State {
public:
  PostingState(std::shared_ptr<Shown const> shown, std::size_t index) : shown_(std::move(shown)), index_(index) {}
  std::string name() const override { return std::to_string(index_); }
  View view() const override {
    auto const& posting = shown_->voucher.postings[index_];
    View lines{std::format("Kontering {} i {}", index_, shown_->id())};
    if (posting.date && *posting.date != shown_->voucher.event.date) lines.push_back(row("Datum", format_iso_date(*posting.date)));
    if (posting.kind == books::Posting::Kind::removed) lines.push_back("  Raden är struken.");
    if (posting.kind == books::Posting::Kind::added) lines.push_back("  Raden är tillagd.");
    if (!posting.source.empty()) {
      // A statement entry's file and line, e.g. "bank/nordea.csv:5"
      auto const colon = posting.source.rfind(':');
      lines.push_back(row("Från", colon == std::string::npos
                                      ? posting.source
                                      : std::format("{} rad {}", posting.source.substr(0, colon),
                                                    posting.source.substr(colon + 1))));
    }
    return lines;
  }
  OptionsList options() const override {
    auto const& posting = shown_->voucher.postings[index_];
    auto const name = shown_->account_name(posting.account);
    return {{U'0', "Konto: " + (name.empty() ? posting.account : posting.account + "  " + name)},
            {U'1', posting.text.empty() ? std::string("Beskrivning:") : "Beskrivning: " + posting.text},
            {U'2', "Belopp: " + format_amount(posting.amount)}};
  }
  // Nothing can be changed yet: each option tells so
  OnKeyResult on_key(CodePoint key) override {
    switch (key) {
    case U'0': return make_design_insufficiency_state("Konto", "att ändra kontot i en kontering");
    case U'1': return make_design_insufficiency_state("Beskrivning", "att ändra beskrivningen i en kontering");
    case U'2': return make_design_insufficiency_state("Belopp", "att ändra beloppet i en kontering");
    default: return nullptr;
    }
  }
  std::string help_topic() const override { return std::string(help_topic::posting); }

private:
  std::shared_ptr<Shown const> shown_;
  std::size_t index_;
};

// The voucher's postings, 0–9 a page at a time
class PostingsState : public State {
public:
  explicit PostingsState(std::shared_ptr<Shown const> shown) : shown_(std::move(shown)) {}
  std::string name() const override { return "Konteringar"; }
  View view() const override {
    auto const count = shown_->voucher.postings.size();
    if (count == 0) return {std::format("{} har inga konteringar.", shown_->id())};
    if (count <= postings_page_rows) return {std::format("Konteringar i {}", shown_->id())};
    return {std::format("Konteringar i {}: {}–{} av {}", shown_->id(), first() + 1, first() + shown(), count)};
  }
  OptionsList options() const override {
    auto const lines = shown_->posting_lines();
    OptionsList options;
    for (std::size_t i = 0; i < shown(); ++i) {
      options.emplace_back(static_cast<CodePoint>(U'0' + i), lines[first() + i]);
    }
    if (lines.size() > postings_page_rows) {
      options.emplace_back(back_key, "Föregående sida");
      options.emplace_back(forward_key, "Nästa sida");
    }
    return options;
  }
  // < and > turn the page in place
  bool on_key_in_place(CodePoint key) override {
    if (key == back_key && page_ > 0) {
      --page_;
      return true;
    }
    if (key == forward_key && first() + postings_page_rows < shown_->voucher.postings.size()) {
      ++page_;
      return true;
    }
    return false;
  }
  OnKeyResult on_key(CodePoint key) override {
    if (key < U'0' || key > U'9') return nullptr;
    auto const i = static_cast<std::size_t>(key - U'0');
    if (i >= shown()) return nullptr;
    return std::make_unique<PostingState>(shown_, first() + i);
  }
  std::string help_topic() const override { return std::string(help_topic::postings); }

private:
  std::size_t first() const { return page_ * postings_page_rows; }
  std::size_t shown() const { return std::min(postings_page_rows, shown_->voucher.postings.size() - first()); }

  std::shared_ptr<Shown const> shown_;
  std::size_t page_ = 0;
};

// Why an option of a booked voucher's event cannot be chosen, and what to do instead
class LockedState : public State {
public:
  LockedState(std::shared_ptr<Shown const> shown, std::string option, std::string what)
      : shown_(std::move(shown)), option_(std::move(option)), what_(std::move(what)) {}
  std::string name() const override { return std::format("Låst ({})", option_); }
  View view() const override {
    auto const& id = shown_->id();
    auto const& series = shown_->voucher.series;
    View lines{std::format("Det går inte att ändra {} i en bokförd verifikation.", what_)};
    if (id == shown_->last_in_series) {
      lines.push_back(std::format("{} är den sista verifikationen i serie {}. Den sista i en serie går att göra om till "
                                  "ett utkast, där du kan ändra händelsen fritt och sedan bokföra den igen: tryck 0 "
                                  "(Gör till utkast) i Bokfört.",
                                  id, series));
    } else {
      auto line = std::string("Bara den sista verifikationen i en serie går att göra om till ett utkast och ändra fritt.");
      if (!shown_->last_in_series.empty()) line += std::format(" I serie {} är det {}.", series, shown_->last_in_series);
      line += std::format(" Är händelsen i {} fel rättar du den med en ny verifikation.", id);
      lines.push_back(std::move(line));
    }
    return lines;
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::string help_topic() const override { return std::string(help_topic::locked); }

private:
  std::shared_ptr<Shown const> shown_;
  std::string option_;
  std::string what_;
};

// The voucher's event: date, heading and amount
class EventState : public State {
public:
  explicit EventState(std::shared_ptr<Shown const> shown) : shown_(std::move(shown)) {}
  std::string name() const override { return "Händelse"; }
  View view() const override { return {std::format("Händelsen i {}", shown_->id())}; }
  OptionsList options() const override {
    auto const& event = shown_->voucher.event;
    auto const amount = shown_->draft && event.amount == 0 ? std::string() : format_amount(event.amount);
    return {{U'0', "Datum: " + format_iso_date(event.date)},
            {U'1', event.heading.empty() ? std::string("Rubrik:") : "Rubrik: " + event.heading},
            {U'2', amount.empty() ? std::string("Belopp:") : "Belopp: " + amount}};
  }
  // The event of a booked voucher is locked: each option tells why, and what to do instead
  // (a draft's event is changed part by part in DraftEventState, see make_draft_view)
  OnKeyResult on_key(CodePoint key) override {
    switch (key) {
    case U'0': return std::make_unique<LockedState>(shown_, "Datum", "datumet");
    case U'1': return std::make_unique<LockedState>(shown_, "Rubrik", "rubriken");
    case U'2': return std::make_unique<LockedState>(shown_, "Belopp", "beloppet");
    default: return nullptr;
    }
  }
  std::string help_topic() const override { return std::string(help_topic::event); }

private:
  std::shared_ptr<Shown const> shown_;
};

// Glöm: the draft forgotten with 9 again (9 is the option that removes); N goes back
class ForgetState : public State {
public:
  ForgetState(std::string id, std::string summary, ForgetDraft forget, State const* origin, State const* draft)
      : id_(std::move(id)), summary_(std::move(summary)), forget_(std::move(forget)), origin_(origin), draft_(draft) {}
  std::string name() const override { return "Glöm"; }
  View view() const override {
    return {std::format("Glöm utkast {}?", id_), "  " + summary_,
            "Utkastet tas bort ur bokföringen och går inte att få tillbaka."};
  }
  OptionsList options() const override {
    if (done_) return {};
    return {{U'9', "Glöm"}, {U'N', "Nej"}};
  }
  OnKeyResult on_key(CodePoint key) override {
    if (done_) return nullptr;
    if (key == U'N') { // keys match exactly (beslut.md)
      declined_ = true;
      return nullptr;
    }
    if (key != U'9' || !forget_) return nullptr;
    if (auto forgotten = forget_(); !forgotten) {
      return pyroceph::tui::make_error_state(std::format("cratchit kunde inte glömma utkastet: {}", forgotten.error()));
    }
    done_ = true;
    return nullptr; // back to Utkast (back_to)
  }
  // Forgotten: back to Utkast, as the draft is gone. N: back to the draft.
  State const* back_to() const override {
    if (done_) return origin_;
    return declined_ ? draft_ : nullptr;
  }
  std::string help_topic() const override { return std::string(help_topic::forget_draft); }

private:
  std::string id_;
  std::string summary_;
  ForgetDraft forget_;
  State const* origin_;
  State const* draft_;
  bool done_ = false;
  bool declined_ = false;
};

// The whole voucher or draft: heading, how it is booked (or noted), event and postings
View view_of(Shown const& shown, std::string const& id) {
  View lines{shown.draft ? std::format("Utkast {} i {} ({})", id, shown.year_label, shown.stage)
                         : "Verifikation " + id};
  auto const booking = shown.booking_rows();
  lines.insert(lines.end(), booking.begin(), booking.end());
  lines.push_back("Händelse:");
  auto const event = shown.event_rows();
  lines.insert(lines.end(), event.begin(), event.end());
  lines.push_back("Konteringar:");
  for (auto const& line : shown.posting_lines()) lines.push_back("  " + line);
  return lines;
}

Shown draft_shown(books::Draft const& draft, std::string id, std::string year_label,
                  std::vector<books::Account> accounts) {
  Shown shown{books::Voucher{draft.series, "", draft.event, std::nullopt, "", draft.postings}, std::move(accounts), "",
              ""};
  shown.draft = true;
  shown.draft_id = std::move(id);
  shown.year_label = std::move(year_label);
  shown.stage = std::string(stage_text(books::stage(draft)));
  shown.noted = draft.noted;
  return shown;
}

class VoucherState : public State {
public:
  explicit VoucherState(std::shared_ptr<Shown const> shown) : shown_(std::move(shown)) {}
  std::string name() const override { return shown_->id(); }
  View view() const override {
    auto lines = view_of(*shown_, name());
    if (shown_->draft) {
      auto const& v = shown_->voucher;
      auto const missing = missing_to_book(books::Draft{v.event, {}, v.postings, v.series});
      lines.insert(lines.end(), missing.begin(), missing.end());
    }
    lines.insert(lines.end(), report_.begin(), report_.end());
    return lines;
  }
  OptionsList options() const override {
    if (split_) return {};
    OptionsList options{{U'0', "Händelse"}, {U'1', "Konteringar"}};
    if (ready()) options.push_back({U'2', "Bokför"});
    if (shown_->draft && shown_->actions.use_template) options.push_back({U'3', "Använd mall"});
    if (!shown_->draft && shown_->reverse) options.push_back({U'2', "Ändringsverifikation"});
    if (shown_->draft && shown_->removal == DraftRemoval::forget) options.push_back({U'9', "Glöm"});
    if (shown_->draft && shown_->removal == DraftRemoval::split) options.push_back({U'9', "Inte samma"});
    return options;
  }
  OnKeyResult on_key(CodePoint key) override {
    if (split_) return nullptr;
    if (shown_->draft && key == U'0' && shown_->actions.edit_event) return shown_->actions.edit_event(this);
    if (shown_->draft && key == U'1' && shown_->actions.edit_postings) return shown_->actions.edit_postings();
    if (key == U'2' && ready()) return shown_->actions.book(this);
    if (key == U'3' && shown_->draft && shown_->actions.use_template) return shown_->actions.use_template(this);
    if (key == U'0') return std::make_unique<EventState>(shown_);
    if (key == U'1') return std::make_unique<PostingsState>(shown_);
    if (key != U'9' || !shown_->draft) return nullptr;
    if (shown_->removal == DraftRemoval::forget) {
      return make_forget_draft_state(shown_->id(), shown_->summary, shown_->actions.forget, shown_->origin, this);
    }
    if (shown_->removal != DraftRemoval::split || !shown_->actions.split) return nullptr;
    if (auto done = shown_->actions.split(); !done) {
      return pyroceph::tui::make_error_state(std::format("cratchit kunde inte dela utkastet: {}", done.error()));
    }
    split_ = true;
    return nullptr; // back to Utkast (back_to)
  }
  // Ändringsverifikation: made at once (a draft, to book or forget), told below the voucher
  bool on_key_in_place(CodePoint key) override {
    if (key != U'2' || shown_->draft || !shown_->reverse) return false;
    auto const made = shown_->reverse();
    report_ = made ? *made : View{std::format("cratchit kunde inte skapa ändringsverifikationen: {}", made.error())};
    return true;
  }
  // Split: the draft is gone, back to Utkast
  State const* back_to() const override { return split_ ? shown_->origin : nullptr; }
  // A draft may have been changed in Kontera: show it as it is now
  void on_return() override {
    if (!shown_->draft || !shown_->actions.now) return;
    auto const now = shown_->actions.now();
    if (!now) return;
    auto shown = draft_shown(now->draft, now->id, shown_->year_label, shown_->accounts);
    shown.summary = now->summary;
    shown.origin = shown_->origin;
    shown.removal = removal_of(now->draft);
    shown.actions = shown_->actions;
    shown_ = std::make_shared<Shown const>(std::move(shown));
  }
  std::string help_topic() const override {
    return std::string(shown_->draft ? help_topic::draft : help_topic::voucher);
  }

private:
  // A draft that is ready, and can be booked from here
  bool ready() const {
    return shown_->draft && shown_->actions.book && shown_->stage == stage_text(books::DraftStage::ready);
  }

  std::shared_ptr<Shown const> shown_;
  bool split_ = false;
  View report_; // what 2 Ändringsverifikation did
};

} // namespace

View missing_to_book(books::Draft const& draft) {
  if (books::stage(draft) == books::DraftStage::ready) return {};
  View lines{"Innan utkastet kan bokföras:"};
  if (draft.event.heading.empty()) lines.push_back("  Rubrik saknas: tryck 0 och sedan 1.");
  if (draft.event.amount == 0) lines.push_back("  Belopp saknas: tryck 0 och sedan 2.");
  auto const counted = [](books::Posting const& p) { return p.kind != books::Posting::Kind::removed; };
  if (std::ranges::none_of(draft.postings, counted)) {
    lines.push_back("  Konteringar saknas: tryck 1, eller 3 för en mall.");
    return lines;
  }
  if (std::ranges::any_of(draft.postings, [&](books::Posting const& p) { return counted(p) && p.account.empty(); })) {
    lines.push_back("  En kontering saknar konto: tryck 1.");
  }
  auto const sum = books::sum(draft.postings);
  auto const amount = draft.event.amount < 0 ? -draft.event.amount : draft.event.amount;
  if (sum != 0) {
    lines.push_back(std::format("  Konteringarna balanserar inte, kvar {} {}: tryck 1.", sum < 0 ? "debet" : "kredit",
                                format_amount(sum < 0 ? -sum : sum)));
  } else if (amount != 0 && books::debit(draft.postings) != amount) {
    lines.push_back(std::format("  Debet {} är inte beloppet {}: tryck 0 och sedan 2, eller 1.",
                                format_amount(books::debit(draft.postings)), format_amount(amount)));
  }
  return lines;
}

View draft_view_lines(books::Draft const& draft, std::string const& id, std::string year_label,
                      std::vector<books::Account> accounts) {
  return view_of(draft_shown(draft, id, std::move(year_label), std::move(accounts)), id);
}

std::unique_ptr<State> make_forget_draft_state(std::string id, std::string summary, ForgetDraft forget,
                                               State const* forgotten_to, State const* declined_to) {
  return std::make_unique<ForgetState>(std::move(id), std::move(summary), std::move(forget), forgotten_to, declined_to);
}

std::unique_ptr<State> make_voucher_menu(books::Voucher voucher, std::vector<books::Account> accounts,
                                         std::string series_name, std::string last_in_series, ReverseVoucher reverse) {
  Shown shown{std::move(voucher), std::move(accounts), std::move(series_name), std::move(last_in_series)};
  shown.reverse = std::move(reverse);
  return std::make_unique<VoucherState>(std::make_shared<Shown const>(std::move(shown)));
}

std::unique_ptr<State> make_draft_view(books::Draft const& draft, std::string id, std::string year_label,
                                       std::vector<books::Account> accounts, std::string summary, State const* origin,
                                       DraftActions actions) {
  auto shown = draft_shown(draft, std::move(id), std::move(year_label), std::move(accounts));
  shown.summary = std::move(summary);
  shown.origin = origin;
  shown.removal = removal_of(draft);
  shown.actions = std::move(actions);
  return std::make_unique<VoucherState>(std::make_shared<Shown const>(std::move(shown)));
}

} // namespace cratchit::ui
