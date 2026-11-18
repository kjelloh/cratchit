#include "ui/draft_menu.hpp"

#include "charset/utf8.hpp"
#include "core/books/books.hpp"
#include "core/date.hpp"
#include "core/event_draft.hpp"
#include "core/pdf_documents.hpp"
#include "core/same_event.hpp"
#include "core/templates.hpp"
#include "core/log.hpp"
#include "core/sie_summary.hpp"
#include "core/statement_drafts.hpp"
#include "pyroceph/tui/error_state.hpp"
#include "pyroceph/tui/path_completion.hpp"
#include "pyroceph/tui/pager.hpp"
#include "pyroceph/tui/prompt_loop.hpp"
#include "ui/design_insufficiency.hpp"
#include "ui/booked_menu.hpp"
#include "ui/unbook_menu.hpp"
#include "ui/help_topics.hpp"
#include "ui/voucher_menu.hpp"
#include "ui/year_view.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <expected>
#include <filesystem>
#include <functional>
#include <limits>
#include <format>
#include <optional>
#include <string>
#include <system_error>
#include <utility>

namespace cratchit::ui {

namespace {

namespace fs = std::filesystem;

using pyroceph::tui::Accepted;
using pyroceph::tui::CodePoint;
using pyroceph::tui::Completion;
using pyroceph::tui::InputResult;
using pyroceph::tui::OnKeyResult;
using pyroceph::tui::OptionsList;
using pyroceph::tui::State;
using pyroceph::tui::View;

constexpr CodePoint back_key = U'<';
constexpr CodePoint forward_key = U'>';

// The new entries Bekräfta lists at most
constexpr std::size_t listed_entries = 20;

std::string utf8_of(std::u32string_view text) {
  std::string result;
  charset::utf8::Encoder encoder;
  for (auto const code_point : text) {
    for (auto const byte : encoder.push(code_point)) result.push_back(static_cast<char>(byte));
  }
  return result;
}

std::u32string u32_of(std::string_view text) { return std::u32string(text.begin(), text.end()); } // ASCII only

// UTF-8 text as code points (a malformed byte stands for itself)
std::u32string code_points_of(std::string_view text) {
  std::u32string result;
  for (std::size_t i = 0; i < text.size();) {
    auto const c = static_cast<unsigned char>(text[i]);
    std::size_t const length = c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : (c >> 3) == 0x1E ? 4 : 1;
    if (length == 1 || i + length > text.size()) {
      result.push_back(c);
      ++i;
      continue;
    }
    char32_t point = c & (0x7F >> length);
    for (std::size_t k = 1; k < length; ++k) point = (point << 6) | (static_cast<unsigned char>(text[i + k]) & 0x3F);
    result.push_back(point);
    i += length;
  }
  return result;
}

// "2026-07-03     -3.70  AVGIFTER NORDEA"
std::string entry_text(statement::Entry const& entry) {
  return std::format("    {}  {:>12}  {}", format_iso_date(entry.date), format_amount(entry.amount), entry.text);
}

// The account with its name, e.g. "1930 Företagskonto"
std::string account_text(books::Books const& books, std::string const& number) {
  auto const* account = books.account(number);
  return account != nullptr && !account->name.empty() ? std::format("{} {}", number, account->name) : number;
}

class ConfirmStatementState : public State {
public:
  ConfirmStatementState(Project project, FiscalYear year, StatementFile statement, StatementDraftPlan plan,
                        std::string account_text, State const* origin, State const* chooser, StatementReport report)
      : project_(std::move(project)), year_(std::move(year)), statement_(std::move(statement)), plan_(std::move(plan)),
        account_text_(std::move(account_text)), origin_(origin), chooser_(chooser), report_(std::move(report)) {}
  std::string name() const override { return "Bekräfta"; }
  View view() const override {
    View lines{std::format("Kontoutdrag: {}", statement_.file.string())};
    auto const row = [&lines](std::string_view label, std::string const& value) {
      lines.push_back(std::format("  {:<16}{}", std::string(label), value));
    };
    row("Konto", account_text_);
    row("Nya utkast", std::to_string(plan_.added.size()));
    row("Redan bokförda", std::to_string(plan_.booked.size()));
    row("Redan utkast", std::to_string(plan_.drafted.size()));
    if (!plan_.outside.empty()) {
      row("Utanför året", std::format("{} (läses inte in)", plan_.outside.size()));
    }
    if (plan_.added.empty()) {
      lines.push_back("Inget nytt att läsa in.");
      return lines;
    }
    lines.push_back("  Nya:");
    auto const shown = std::min(plan_.added.size(), listed_entries);
    for (std::size_t i = 0; i < shown; ++i) lines.push_back(entry_text(plan_.added[i]));
    if (shown < plan_.added.size()) lines.push_back(std::format("    … och {} till", plan_.added.size() - shown));
    lines.push_back(plan_.added.size() == 1 ? "Skapa 1 utkast?" : std::format("Skapa {} utkast?", plan_.added.size()));
    return lines;
  }
  OptionsList options() const override {
    if (done_) return {};
    if (plan_.added.empty()) return {{U'N', "Tillbaka"}};
    return {{U'J', "Ja"}, {U'N', "Nej"}};
  }
  OnKeyResult on_key(CodePoint key) override {
    if (done_) return nullptr;
    if (key == U'N') { // keys match exactly (beslut.md)
      declined_ = true;
      return nullptr;
    }
    if (key != U'J' || plan_.added.empty()) return nullptr;
    auto const start = parse_iso_date(year_.start);
    if (!start) return pyroceph::tui::make_error_state(std::format("Felaktigt räkenskapsår {}", year_.start));
    auto const added = add_statement_drafts(project_.folder, statement_.file, plan_, *start, today_date());
    if (!added) {
      LOG_BUSINESS("Kunde inte läsa in {} som utkast i {}: {}", statement_.file.string(), project_.folder.string(),
                   added.error());
      return pyroceph::tui::make_error_state(std::format("cratchit kunde inte skapa utkasten: {}", added.error()));
    }
    LOG_BUSINESS("Kontoutdrag {} inläst för konto {} i {}: {} nya utkast, {} redan bokförda, {} redan utkast, {} "
                 "utanför året",
                 statement_.file.string(), plan_.account, label_of(year_), *added, plan_.booked.size(),
                 plan_.drafted.size(), plan_.outside.size());
    done_ = true;
    if (report_) {
      report_({std::format("cratchit skapade {} utkast från {} på konto {}.", *added, statement_.file.filename().string(),
                           account_text_)},
              statement_.file);
    }
    return nullptr; // back to Utkast (back_to)
  }
  // Done: back to Utkast. N: back to choose another file.
  State const* back_to() const override {
    if (done_) return origin_;
    return declined_ ? chooser_ : nullptr;
  }
  std::string help_topic() const override { return std::string(help_topic::confirm_statement); }

private:
  Project project_;
  FiscalYear year_;
  StatementFile statement_;
  StatementDraftPlan plan_;
  std::string account_text_;
  State const* origin_;
  State const* chooser_;
  StatementReport report_;
  bool done_ = false;
  bool declined_ = false;
};

class StatementAccountState : public State {
public:
  StatementAccountState(Project project, FiscalYear year, StatementFile statement, books::Books books,
                        State const* origin, State const* chooser, StatementReport report)
      : project_(std::move(project)), year_(std::move(year)), statement_(std::move(statement)), books_(std::move(books)),
        origin_(origin), chooser_(chooser), report_(std::move(report)),
        guess_(guess_statement_account(books_, statement_.document)) {}
  std::string name() const override { return "Konto"; }
  View view() const override {
    auto const& document = statement_.document;
    View lines{std::format("Kontoutdrag: {}", statement_.file.string())};
    auto const row = [&lines](std::string_view label, std::string const& value) {
      lines.push_back(std::format("  {:<10}{}", std::string(label), value));
    };
    row("Kodning", document.encoding);
    auto const order = document.order == statement::Order::newest_first   ? ", nyast först"
                       : document.order == statement::Order::oldest_first ? ", äldst först"
                                                                          : "";
    row("Rader", std::format("{}, {} – {} (rad {}–{}{})", document.entries.size(),
                             format_iso_date(document.entries.front().date),
                             format_iso_date(document.entries.back().date), document.first_line, document.last_line,
                             order));
    if (document.saldo_column && document.opening && document.closing) {
      row("Saldo", std::format("{} före, {} efter, stämmer på varje rad som har saldo", format_amount(*document.opening),
                               format_amount(*document.closing)));
    }
    for (auto const& note : document.notes) lines.push_back("  " + note);
    if (guess_) {
      lines.push_back(std::format("Förslag: {} ({} av {} rader stämmer med konteringar på {} i bokföringen).",
                                  account_text(books_, guess_->account), guess_->matches, document.entries.size(),
                                  guess_->account));
    }
    lines.push_back("Skriv BAS-kontot som kontoutdraget gäller och tryck Enter.");
    return lines;
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::u32string start_input() const override { return guess_ ? u32_of(guess_->account) : std::u32string(); }
  InputResult on_input(std::u32string_view text) override {
    auto const typed = utf8_of(text);
    auto const first = typed.find_first_not_of(' ');
    auto const number = first == std::string::npos ? std::string() : typed.substr(first, typed.find_last_not_of(' ') - first + 1);
    if (number.empty()) return std::unexpected(std::string("Skriv kontots nummer, till exempel 1930."));
    if (books_.account(number) == nullptr) {
      return std::unexpected(std::format("Konto {} finns inte i bokföringens kontoplan.", number));
    }
    auto const start = parse_iso_date(year_.start);
    auto const end = parse_iso_date(year_.end);
    if (!start || !end) return std::unexpected(std::format("Felaktigt räkenskapsår {} – {}", year_.start, year_.end));
    // A year not in the books yet has nothing booked: it is added with the drafts (continued_year)
    auto const* year = books_.year_starting(*start);
    books::FiscalYear const empty{*start, *end};
    auto plan = plan_statement_drafts(year != nullptr ? *year : empty, statement_.document, number);
    LOG_BUSINESS("Kontoutdrag {}: konto {} valt", statement_.file.string(), number);
    return Accepted(std::make_unique<ConfirmStatementState>(project_, year_, statement_, std::move(plan),
                                                            account_text(books_, number), origin_, chooser_, report_));
  }
  std::string help_topic() const override { return std::string(help_topic::statement_account); }

private:
  Project project_;
  FiscalYear year_;
  StatementFile statement_;
  books::Books books_;
  State const* origin_;
  State const* chooser_;
  StatementReport report_;
  std::optional<AccountGuess> guess_;
};

class ReadStatementState : public State {
public:
  ReadStatementState(Project project, FiscalYear year, State const* origin, StatementReport report,
                     std::optional<fs::path> start_folder)
      : project_(std::move(project)), year_(std::move(year)), origin_(origin), report_(std::move(report)),
        chosen_folder_(std::move(start_folder)) {}
  std::string name() const override { return "Läs kontoutdrag"; }
  View view() const override {
    return {"Skriv sökvägen till kontoutdraget och tryck Enter. Tab fyller i mappar och filer."};
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  // The folder of the file chosen last, so that going back offers its neighbours
  std::u32string start_input() const override {
    return pyroceph::tui::path_input_of(chosen_folder_.value_or(project_.folder));
  }
  // All folders and files: cratchit does not judge what a statement may be
  Completion complete(std::u32string_view text) const override { return pyroceph::tui::complete_path(text, true); }
  InputResult on_input(std::u32string_view text) override {
    auto const file = pyroceph::tui::path_of(text);
    if (file.empty()) return std::unexpected(std::string("Skriv sökvägen till kontoutdraget."));
    std::error_code ec;
    if (fs::is_directory(file, ec)) {
      return std::unexpected(std::format("{} är en mapp. Välj en fil i den med Tab.", file.string()));
    }
    if (!fs::is_regular_file(file, ec)) return std::unexpected(std::format("Det finns ingen fil {}.", file.string()));
    chosen_folder_ = file.parent_path();
    auto statement = read_statement_file(file);
    if (!statement) {
      LOG_BUSINESS("Kan inte läsa kontoutdrag {}: {}", file.string(), statement.error());
      return std::unexpected(std::format("cratchit kan inte läsa filen som kontoutdrag. {}", statement.error()));
    }
    auto books = books::read_books(project_.folder);
    if (!books) return std::unexpected(std::format("cratchit kunde inte läsa bokföringen: {}", books.error()));
    LOG_BUSINESS("Kontoutdrag läst för {} i {}: {} ({} rader, {})", label_of(year_), project_.folder.string(),
                 file.string(), statement->document.entries.size(), statement->document.encoding);
    return Accepted(std::make_unique<StatementAccountState>(project_, year_, std::move(*statement), std::move(*books),
                                                            origin_, this, report_));
  }
  std::string help_topic() const override { return std::string(help_topic::read_statement); }

private:
  Project project_;
  FiscalYear year_;
  State const* origin_;
  StatementReport report_;
  std::optional<fs::path> chosen_folder_;
};

// Reports what was done to the state the TUI goes back to
using EventReport = std::function<void(View)>;

// Adds the event as a draft of year; the error text when it cannot
std::expected<void, std::string> make_event_draft(Project const& project, FiscalYear const& year,
                                                  books::Event const& event) {
  auto const start = parse_iso_date(year.start);
  if (!start) return std::unexpected(std::format("Felaktigt räkenskapsår {}", year.start));
  auto const added = add_event_draft(project.folder, *start, event, today_date());
  if (!added) {
    LOG_BUSINESS("Kunde inte skapa utkast av händelsen {} i {}: {}", event.heading, project.folder.string(),
                 added.error());
    return std::unexpected(std::format("cratchit kunde inte skapa utkastet: {}", added.error()));
  }
  LOG_BUSINESS("Utkast {} skapat i {} ({}): {} {} {}", draft_id(*added, draft_count(project.folder, *start)), label_of(year), project.folder.string(),
               format_iso_date(event.date), format_amount(event.amount), event.heading);
  return {};
}

// Tells Beskriv händelser that a draft was made of the event
using EventCreated = std::function<void(books::Event const&)>;

class AmountChoiceState : public State {
public:
  AmountChoiceState(Project project, FiscalYear year, std::vector<books::Event> readings, State const* describe,
                    EventCreated created)
      : project_(std::move(project)), year_(std::move(year)), readings_(std::move(readings)), describe_(describe),
        created_(std::move(created)) {}
  std::string name() const override { return "Belopp?"; }
  View view() const override { return {"Beloppet är otydligt: mellanslag kan skilja tusental. Vilket menar du?"}; }
  OptionsList options() const override {
    OptionsList result;
    for (std::size_t i = 0; i < readings_.size() && i < 9; ++i) {
      result.push_back({static_cast<CodePoint>(U'1' + i),
                        std::format("{:>12}  {}", format_amount(readings_[i].amount), readings_[i].heading)});
    }
    return result;
  }
  // A reading makes the draft and goes back to Beskriv händelser (back_to), for the next event
  OnKeyResult on_key(CodePoint key) override {
    if (done_ || key < U'1' || key > U'9') return nullptr;
    auto const i = static_cast<std::size_t>(key - U'1');
    if (i >= readings_.size()) return nullptr;
    if (auto made = make_event_draft(project_, year_, readings_[i]); !made) {
      return pyroceph::tui::make_error_state(made.error());
    }
    if (created_) created_(readings_[i]);
    done_ = true;
    return nullptr;
  }
  State const* back_to() const override { return done_ ? describe_ : nullptr; }
  std::string help_topic() const override { return std::string(help_topic::amount_choice); }

private:
  Project project_;
  FiscalYear year_;
  std::vector<books::Event> readings_;
  State const* describe_;
  EventCreated created_;
  bool done_ = false;
};

// Beskriv händelser: one event after the other, each line a draft (beslut 2026-11-17); the
// user stays here, with an empty line, and sees the drafts made so far. - goes back to Utkast.
class DescribeEventState : public State {
public:
  DescribeEventState(Project project, FiscalYear year, EventReport report)
      : project_(std::move(project)), year_(std::move(year)), report_(std::move(report)) {}
  std::string name() const override { return "Beskriv händelser"; }
  View view() const override {
    View lines{"Skriv händelsen: rubrik, belopp och datum, i vilken ordning som helst, och tryck Enter.",
               "Till exempel: Telia faktura oktober 1118,00 2026-10-22",
               std::format("I dag är {}. Räkenskapsåret är {} – {}.", format_iso_date(today_date()), year_.start,
                           year_.end)};
    if (!created_.empty()) {
      lines.push_back("");
      lines.push_back("Skapade nu:");
      for (auto const& event : created_) {
        lines.push_back(std::format("  {}  {:>12}  {}", format_iso_date(event.date), format_amount(event.amount),
                                    event.heading));
      }
    }
    return lines;
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  // The line typed last, kept when it was refused or the amount was unclear (- from Belopp?
  // comes back to it); empty once a draft is made of it
  std::u32string start_input() const override { return line_; }
  InputResult on_input(std::u32string_view text) override {
    line_ = text;
    auto readings = parse_event(utf8_of(text));
    if (!readings) return std::unexpected(readings.error());
    auto const start = parse_iso_date(year_.start);
    auto const end = parse_iso_date(year_.end);
    auto const date = readings->front().date;
    if (start && end && (date < *start || date > *end)) {
      return std::unexpected(std::format("{} ligger utanför räkenskapsåret {} – {}.", format_iso_date(date),
                                         year_.start, year_.end));
    }
    if (readings->size() > 1) {
      return Accepted(std::make_unique<AmountChoiceState>(project_, year_, std::move(*readings), this,
                                                          [this](books::Event const& event) { created(event); }));
    }
    // A clear line makes the draft at once, without confirmation (beslut 2026-11-17); stay for the next
    if (auto made = make_event_draft(project_, year_, readings->front()); !made) {
      return std::unexpected(made.error());
    }
    created(readings->front());
    return Accepted();
  }
  std::string help_topic() const override { return std::string(help_topic::describe_event); }

private:
  // A draft made of event: listed here, the line emptied, and Utkast told how many so far
  void created(books::Event const& event) {
    created_.push_back(event);
    line_.clear();
    if (report_) {
      report_({created_.size() == 1 ? std::format("cratchit skapade 1 utkast: {}.", event.heading)
                                    : std::format("cratchit skapade {} utkast.", created_.size())});
    }
  }

  Project project_;
  FiscalYear year_;
  EventReport report_;
  std::u32string line_;
  std::vector<books::Event> created_;
};

// What Tab tells about the pdf files of a folder, e.g. "3 pdf-filer i mappen:" and their names
std::vector<std::string> pdf_lines(fs::path const& folder) {
  auto const files = pdf_files_in(folder);
  if (files.empty()) return {"Inga pdf-filer i mappen."};
  std::vector<std::string> lines{files.size() == 1 ? std::string("1 pdf-fil i mappen:")
                                                   : std::format("{} pdf-filer i mappen:", files.size())};
  for (auto const& file : files) {
    auto const name = file.filename().u8string();
    lines.push_back("  " + std::string(name.begin(), name.end()));
  }
  return lines;
}

// Code points in UTF-8 text, for lining up columns
std::size_t width_of(std::string const& text) {
  return static_cast<std::size_t>(
      std::ranges::count_if(text, [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; }));
}

// The pdf files a list shows its names to at most, before the status
constexpr std::size_t pdf_name_width = 50;

// The pdf files of a folder, numbered, each with what walk_pdf finds in it
// (see status_text): "   1  faktura.pdf          text, 1 sida"
// The pdf files of a folder, each read once (read_pdf) when the folder is chosen, shared
// by Pdf-underlag, Underlag and Tolka
using Documents = std::shared_ptr<std::vector<PdfDocument> const>;

std::vector<std::string> pdf_rows(std::vector<PdfDocument> const& documents) {
  std::vector<std::string> names;
  std::size_t width = 0;
  for (auto const& document : documents) {
    auto const name = document.file.filename().u8string();
    names.emplace_back(name.begin(), name.end());
    width = std::max(width, std::min(width_of(names.back()), pdf_name_width));
  }
  std::vector<std::string> rows;
  for (std::size_t i = 0; i < documents.size(); ++i) {
    auto const shown = width_of(names[i]);
    auto const padding = shown < width ? std::string(width - shown, ' ') : std::string();
    rows.push_back(std::format("  {:>3}  {}{}  {}", i + 1, names[i], padding, status_text(documents[i].status)));
  }
  return rows;
}

// The lines a document shows at most, for now (invoices are short)
constexpr std::size_t document_lines = 20;

// The pages of a pdf file's text, each its lines; none without text
using Pages = std::vector<std::vector<std::string>>;

// The width of the lines that frame a document's text
constexpr std::size_t frame_width = 68;

// A frame line with a label in the middle, e.g. "==== sida 1 av 2 ====", or without one
std::string frame_line(char c, std::string_view label = {}) {
  if (label.empty()) return std::string(frame_width, c);
  auto const text = std::format(" {} ", label);
  auto const left = (frame_width - text.size()) / 2;
  return std::string(left, c) + text + std::string(frame_width - left - text.size(), c);
}

// The lines inside the frame: each page's lines, a "---- sida n av m ----" line between pages
std::vector<std::string> framed_body(Pages const& pages) {
  std::vector<std::string> body;
  for (std::size_t page = 0; page < pages.size(); ++page) {
    if (page > 0) body.push_back(frame_line('-', std::format("sida {} av {}", page + 1, pages.size())));
    body.insert(body.end(), pages[page].begin(), pages[page].end());
  }
  return body;
}

// A document's text in its frame, with a blank line before: "==== sida 1 av m ====", at most
// max_lines lines of the body, "====", then "… N rader till" when cut. Nothing without pages.
std::vector<std::string> framed_text(Pages const& pages, std::size_t max_lines) {
  if (pages.empty()) return {};
  auto const body = framed_body(pages);
  std::vector<std::string> lines{"", frame_line('=', std::format("sida 1 av {}", pages.size()))};
  auto const shown = std::min(body.size(), max_lines);
  lines.insert(lines.end(), body.begin(), body.begin() + static_cast<std::ptrdiff_t>(shown));
  lines.push_back(frame_line('='));
  if (shown < body.size()) lines.push_back(std::format("… {} rader till", body.size() - shown));
  return lines;
}

std::size_t line_count(Pages const& pages) {
  std::size_t count = 0;
  for (auto const& page : pages) count += page.size();
  return count;
}

// The name, cut to at most max code points with "…" at the end
std::string cut_name(std::string const& name, std::size_t max) {
  if (width_of(name) <= max) return name;
  std::string cut;
  std::size_t count = 0;
  for (std::size_t i = 0; i < name.size(); ++i) {
    bool const starts = (static_cast<unsigned char>(name[i]) & 0xC0) != 0x80;
    if (starts && ++count > max - 1) break;
    cut += name[i];
  }
  return cut + "…";
}

std::string file_name_of(fs::path const& file) {
  auto const name = file.filename().u8string();
  return {name.begin(), name.end()};
}

// Tolka: the whole text of a document, to make an event of it later. No options
// for now: - goes back to the document.
class InterpretState : public State {
public:
  InterpretState(Documents documents, std::size_t index) : documents_(std::move(documents)), index_(index) {}
  std::string name() const override { return "Tolka"; }
  View view() const override {
    auto const& document = (*documents_)[index_];
    auto const& pages = document.pages;
    View lines{std::format("Tolka {} ({}{})", file_name_of(document.file), status_text(document.status),
                           pages.empty() ? std::string() : std::format(", {} rader", line_count(pages)))};
    if (pages.empty()) {
      lines.push_back("");
      lines.push_back("Ingen text att tolka.");
    } else {
      auto const text = framed_text(pages, std::numeric_limits<std::size_t>::max());
      lines.insert(lines.end(), text.begin(), text.end());
    }
    lines.push_back("");
    lines.push_back("Att göra en händelse av underlaget finns inte än.");
    return lines;
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::string help_topic() const override { return std::string(help_topic::interpret); }

private:
  Documents documents_;
  std::size_t index_;
};

// Underlag <n>: the pdf files of a folder one at a time, from the first: its
// text, page by page, at most document_lines lines. < and > go to the one
// before and after (in place; at either end they do nothing). 1 Tolka shows
// its whole text (InterpretState); 0 Glöm leads to Inte än for now.
class DocumentState : public State {
public:
  explicit DocumentState(Documents documents) : documents_(std::move(documents)) {}
  std::string name() const override { return std::format("Underlag {}", index_ + 1); }
  View view() const override {
    auto const& document = (*documents_)[index_];
    View lines{std::format("Underlag {} av {}: {} ({})", index_ + 1, documents_->size(), file_name_of(document.file),
                           status_text(document.status))};
    if (document.pages.empty()) {
      lines.push_back("");
      lines.push_back("Ingen text att visa.");
    } else {
      auto const text = framed_text(document.pages, document_lines);
      lines.insert(lines.end(), text.begin(), text.end());
    }
    lines.push_back("");
    return lines;
  }
  OptionsList options() const override {
    return {{back_key, "Föregående"}, {U'0', "Glöm"}, {U'1', "Tolka"}, {forward_key, "Nästa"}};
  }
  bool on_key_in_place(CodePoint key) override {
    if (key == back_key && index_ > 0) {
      --index_;
      return true;
    }
    if (key == forward_key && index_ + 1 < documents_->size()) {
      ++index_;
      return true;
    }
    return false;
  }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'0') return make_design_insufficiency_state("Glöm", "att glömma ett underlag");
    if (key == U'1') return std::make_unique<InterpretState>(documents_, index_);
    return nullptr;
  }
  std::string help_topic() const override { return std::string(help_topic::document); }

private:
  Documents documents_;
  std::size_t index_ = 0;
};

// Pdf-underlag: the pdf files of a folder, numbered, each with a status line
// telling whether walk_pdf finds text in it; a page at a time. 0 Tolka som
// händelser goes through them one at a time (DocumentState).
class PdfListState : public State {
public:
  PdfListState(fs::path folder, Documents documents) : folder_(std::move(folder)), documents_(std::move(documents)) {
    pager_.set(pdf_rows(*documents_));
  }
  std::string name() const override { return "Pdf-underlag"; }
  View view() const override {
    if (documents_->empty()) return {std::format("Inga pdf-filer i {}.", folder_.string())};
    View lines{std::format("Pdf-underlag i {}: {}", folder_.string(),
                           documents_->size() == 1 ? std::string("1 fil")
                                                   : std::format("{} filer", documents_->size()))};
    auto const page = pager_.view();
    lines.insert(lines.end(), page.begin(), page.end());
    return lines;
  }
  OptionsList options() const override {
    if (documents_->empty()) return {};
    return {{U'0', "Tolka som händelser"}, {back_key, "Föregående sida"}, {forward_key, "Nästa sida"}};
  }
  bool on_key_in_place(CodePoint key) override {
    if (key == back_key) return pager_.back();
    if (key == forward_key) return pager_.forward();
    return false;
  }
  OnKeyResult on_key(CodePoint key) override {
    if (key != U'0' || documents_->empty()) return nullptr;
    return std::make_unique<DocumentState>(documents_); // the text read with the list: nothing is read again
  }
  std::string help_topic() const override { return std::string(help_topic::pdf_list); }

private:
  fs::path folder_;
  Documents documents_;
  pyroceph::tui::Pager pager_{booked_page_rows};
};

class ReadPdfState : public State {
public:
  ReadPdfState(Project project, FiscalYear year, std::optional<fs::path> start_folder,
               std::function<void(fs::path const&)> remember)
      : project_(std::move(project)), year_(std::move(year)), chosen_folder_(std::move(start_folder)),
        remember_(std::move(remember)) {}
  std::string name() const override { return "Läs pdf-underlag"; }
  View view() const override {
    return {"Skriv sökvägen till mappen med pdf-underlagen och tryck Enter. Tab fyller i mappar och visar mappens "
            "pdf-filer."};
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::u32string start_input() const override {
    return pyroceph::tui::path_input_of(chosen_folder_.value_or(project_.folder));
  }
  // Folders only; the pdf files of the folder reached are shown as information, not to choose
  Completion complete(std::u32string_view text) const override {
    auto completion = pyroceph::tui::complete_path(text, false);
    std::u32string completed(text);
    completed += completion.added;
    auto const path = pyroceph::tui::path_of(completed);
    auto const ends_in_folder = !completed.empty() && pyroceph::tui::path_input_of(path).back() == completed.back();
    auto const folder = ends_in_folder ? path : path.parent_path();
    std::error_code ec;
    if (!fs::is_directory(folder, ec)) return completion;
    auto const lines = pdf_lines(folder);
    if (!completion.candidates.empty()) {
      completion.candidates.push_back("");
      completion.candidates.insert(completion.candidates.end(), lines.begin(), lines.end());
    } else {
      completion.notes.insert(completion.notes.end(), lines.begin(), lines.end());
    }
    return completion;
  }
  InputResult on_input(std::u32string_view text) override {
    auto folder = pyroceph::tui::path_of(text);
    if (folder.empty()) return std::unexpected(std::string("Skriv sökvägen till mappen."));
    if (!folder.has_filename() && folder.has_parent_path() && folder != folder.root_path()) {
      folder = folder.parent_path(); // without the separator at the end
    }
    std::error_code ec;
    if (fs::is_regular_file(folder, ec)) {
      return std::unexpected(std::format("{} är en fil. Välj mappen den ligger i.", folder.string()));
    }
    if (!fs::is_directory(folder, ec)) return std::unexpected(std::format("Det finns ingen mapp {}.", folder.string()));
    chosen_folder_ = folder;
    if (remember_) remember_(folder);
    // Each file read once, its status and text kept for the list, Underlag and Tolka
    std::vector<PdfDocument> documents;
    std::size_t with_text = 0;
    auto const files = pdf_files_in(folder);
    for (std::size_t i = 0; i < files.size(); ++i) {
      // The name cut, so that the line fits a narrow terminal and can be written over
      pyroceph::tui::show_progress(
          std::format("Läser pdf-filer: {} av {} ({})", i + 1, files.size(), cut_name(file_name_of(files[i]), 40)));
      documents.push_back(read_pdf(files[i]));
      with_text += documents.back().status.kind == PdfStatus::Kind::text ? 1 : 0;
    }
    if (!files.empty()) {
      pyroceph::tui::show_progress(std::format("Läste {} pdf-filer, {} med text.", files.size(), with_text));
    }
    LOG_BUSINESS("Pdf-underlag lästa för {} i {}: {} ({} pdf-filer, {} med text)", label_of(year_),
                 project_.folder.string(), folder.string(), documents.size(), with_text);
    return Accepted(std::make_unique<PdfListState>(
        folder, std::make_shared<std::vector<PdfDocument> const>(std::move(documents))));
  }
  std::string help_topic() const override { return std::string(help_topic::read_pdf); }

private:
  Project project_;
  FiscalYear year_;
  std::optional<fs::path> chosen_folder_;
  std::function<void(fs::path const&)> remember_;
};

// Forgets the draft at index of year, the one seen, and logs it with its postings (why: e.g. "glömt")
std::expected<void, std::string> forget_logged(Project const& project, FiscalYear const& year, std::size_t index,
                                               std::size_t count, books::Draft const& draft, std::string_view why) {
  auto const id = draft_id(index, count);
  auto const year_start = parse_iso_date(year.start);
  if (!year_start) return std::unexpected(std::format("Felaktigt räkenskapsår {}", year.start));
  if (auto forgotten = forget_draft(project.folder, *year_start, index, draft); !forgotten) {
    LOG_BUSINESS("Kunde inte glömma utkast {} i {} ({}): {}", id, label_of(year), project.folder.string(),
                 forgotten.error());
    return forgotten;
  }
  LOG_BUSINESS("Utkast {} {} i {} ({}): {} {} \"{}\", {} konteringar", id, why, label_of(year), project.folder.string(),
               format_iso_date(draft.event.date), format_amount(draft.event.amount), draft.event.heading,
               draft.postings.size());
  for (auto const& posting : draft.postings) {
    LOG_BUSINESS("  glömd kontering: {} {} {} \"{}\" {}", posting.account, format_amount(posting.amount),
                 posting.date ? format_iso_date(*posting.date) : std::string(), posting.text, posting.source);
  }
  return {};
}

// Splits the draft at index of year, the one seen (split_draft), and logs it; gives how many drafts it became
std::expected<std::size_t, std::string> split_logged(Project const& project, FiscalYear const& year, std::size_t index,
                                                     std::size_t drafts, books::Draft const& draft) {
  auto const id = draft_id(index, drafts);
  auto const year_start = parse_iso_date(year.start);
  if (!year_start) return std::unexpected(std::format("Felaktigt räkenskapsår {}", year.start));
  auto const count = split_draft(project.folder, *year_start, index, draft);
  if (!count) {
    LOG_BUSINESS("Kunde inte dela utkast {} i {} ({}): {}", id, label_of(year), project.folder.string(), count.error());
    return count;
  }
  LOG_BUSINESS("Utkast {} delat i {} ({}): {} {} \"{}\", {} konteringar, blev {} utkast", id, label_of(year),
               project.folder.string(), format_iso_date(draft.event.date), format_amount(draft.event.amount),
               draft.event.heading, draft.postings.size(), *count);
  return count;
}

// A row of draft_rows or voucher_rows without its indent
std::string unindented(std::string row) { return row.starts_with("  ") ? row.substr(2) : row; }

// "samma dag", "1 dag emellan", "3 dagar emellan"
std::string days_text(int days) {
  if (days == 0) return "samma dag";
  return days == 1 ? "1 dag emellan" : std::format("{} dagar emellan", days);
}

// What a pair has going for it: "2 dagar emellan, delar orden: loopia"; "passar, ..." for
// a draft whose postings are all in the voucher (two drafts listed always fit)
std::string pair_note(SameEvent const& pair) {
  std::string note = pair.kind == SameEvent::Kind::booked && pair.fits ? "passar, " : "";
  note += days_text(pair.days);
  if (!pair.words.empty()) {
    note += ", delar orden: ";
    for (std::size_t i = 0; i < pair.words.size(); ++i) note += (i == 0 ? "" : ", ") + pair.words[i];
  }
  return note;
}

std::string account_name(std::vector<books::Account> const& accounts, std::string const& number) {
  auto const found = std::ranges::find(accounts, number, &books::Account::number);
  return found != accounts.end() ? found->name : std::string();
}

// A draft's or voucher's event and postings, under a heading line
View event_lines(std::string heading, books::Event const& event, bool draft, books::Postings const& postings,
                 std::vector<books::Account> const& accounts) {
  auto const row = [](std::string_view label, std::string const& value) {
    return std::format("  {:<16}{}", std::string(label) + ":", value);
  };
  View lines{std::move(heading), row("Datum", format_iso_date(event.date)), row("Rubrik", event.heading),
             row("Belopp", draft && event.amount == 0 ? std::string() : format_amount(event.amount))};
  auto const counted = std::ranges::count_if(postings, [](books::Posting const& p) {
    return p.kind != books::Posting::Kind::removed;
  });
  if (counted == 0) {
    lines.push_back(row("Konteringar", "inga"));
    return lines;
  }
  lines.push_back("  Konteringar:");
  for (auto const& posting : postings) {
    if (posting.kind == books::Posting::Kind::removed) continue;
    auto line = std::format("    {}  {}  {:>12}", posting.account, account_name(accounts, posting.account),
                            format_amount(posting.amount));
    if (!posting.text.empty()) line += "  " + posting.text;
    if (posting.date && *posting.date != event.date) line += "  " + format_iso_date(*posting.date);
    lines.push_back(std::move(line));
  }
  return lines;
}

View draft_lines(std::size_t index, std::size_t count, books::Draft const& draft,
                 std::vector<books::Account> const& accounts) {
  return event_lines(std::format("Utkast {} ({}), noterat {}", draft_id(index, count), stage_text(books::stage(draft)),
                                 format_iso_date(draft.noted)),
                     draft.event, true, draft.postings, accounts);
}

// What the year holds that a pair needs: its drafts, vouchers and account names
struct PairBooks {
  std::vector<books::Draft> drafts;
  std::vector<books::Voucher> vouchers;
  std::vector<books::Account> accounts;
};

// <n>: a pair from Samma?, both in full and what holds them together.
// Two drafts: 0 Samma! makes them one (see merge_drafts), with the event of the
// one that has it (at most one has, see find_same_events). A draft and
// a voucher: 9 Glöm forgets the draft as already in the books (make_forget_draft_state).
// Done, back to Samma? (list), which finds the pairs again.
class PairState : public State {
public:
  PairState(std::string number, Project project, FiscalYear year, SameEvent pair, std::shared_ptr<PairBooks const> books,
            State const* list, std::function<void(std::string)> report)
      : number_(std::move(number)), project_(std::move(project)), year_(std::move(year)), pair_(std::move(pair)),
        books_(std::move(books)), list_(list), report_(std::move(report)) {}
  std::string name() const override { return number_; }
  View view() const override {
    auto const& draft = books_->drafts[pair_.draft];
    View lines{std::format("Samma händelse? {}.", pair_note(pair_))};
    auto const append = [&](View const& more) { lines.insert(lines.end(), more.begin(), more.end()); };
    append(draft_lines(pair_.draft, books_->drafts.size(), draft, books_->accounts));
    if (pair_.kind == SameEvent::Kind::drafts) {
      auto const& other = books_->drafts[pair_.other];
      append(draft_lines(pair_.other, books_->drafts.size(), other, books_->accounts));
      if (!draft.postings.empty() && !other.postings.empty()) {
        lines.push_back("Konteringarna balanserar tillsammans: ut från ett konto och in på ett annat.");
      } else {
        auto const& postings = draft.postings.empty() ? other : draft;
        lines.push_back(books::stage(postings) == books::DraftStage::account_event
                            ? "Det ena är en händelse och det andra en kontohändelse."
                            : "Det ena är en händelse och det andra har bara konteringar.");
      }
      return lines;
    }
    auto const& voucher = books_->vouchers[pair_.other];
    auto const id = books::voucher_id(voucher.series, voucher.number);
    append(event_lines("Verifikation " + id, voucher.event, false, voucher.postings, books_->accounts));
    auto const left = unbooked(draft, voucher);
    if (draft.postings.empty()) {
      lines.push_back("Utkastet har inga konteringar.");
    } else if (left.empty()) {
      lines.push_back(std::format("Utkastets konteringar finns alla i {}.", id));
    } else {
      lines.push_back(std::format("Utkastets konteringar som inte finns i {} glöms med utkastet:", id));
      for (auto const& posting : left) {
        lines.push_back(std::format("    {}  {:>12}  {}", posting.account, format_amount(posting.amount), posting.text));
      }
    }
    return lines;
  }
  OptionsList options() const override {
    if (done_) return {};
    if (pair_.kind == SameEvent::Kind::booked) return {{U'9', "Glöm"}};
    return {{U'0', "Samma!"}};
  }
  OnKeyResult on_key(CodePoint key) override {
    if (done_) return nullptr;
    auto const& draft = books_->drafts[pair_.draft];
    if (pair_.kind == SameEvent::Kind::booked) {
      if (key != U'9') return nullptr;
      auto const& voucher = books_->vouchers[pair_.other];
      auto const id = draft_id(pair_.draft, books_->drafts.size());
      auto const voucher_id = books::voucher_id(voucher.series, voucher.number);
      auto forget = [project = project_, year = year_, index = pair_.draft, count = books_->drafts.size(), draft, id, voucher_id,
                     report = report_]() -> std::expected<void, std::string> {
        if (auto forgotten = forget_logged(project, year, index, count, draft, "glömt (redan bokfört som " + voucher_id + ")");
            !forgotten) {
          return forgotten;
        }
        report(std::format("cratchit glömde utkast {}, redan bokfört som {}: {}.", id, voucher_id, draft_text(draft)));
        return {};
      };
      auto summary = unindented(draft_row(pair_.draft, books_->drafts.size(), draft));
      summary = summary.substr(summary.find(format_iso_date(draft.event.date)));
      return make_forget_draft_state(id, summary, std::move(forget), list_, this);
    }
    auto const& other = books_->drafts[pair_.other];
    if (key != U'0') return nullptr;
    // At most one has an event (find_same_events): the one draft gets it
    auto const has_event = [](books::Draft const& d) { return !d.event.heading.empty() || d.event.amount != 0; };
    auto const event = has_event(other) ? other.event : draft.event;
    auto const year_start = parse_iso_date(year_.start);
    if (!year_start) return pyroceph::tui::make_error_state(std::format("Felaktigt räkenskapsår {}", year_.start));
    auto const count = books_->drafts.size();
    auto const a = draft_id(pair_.draft, count);
    auto const b = draft_id(pair_.other, count);
    auto const index = merge_drafts(project_.folder, *year_start, pair_.draft, draft, pair_.other, other, event);
    if (!index) {
      LOG_BUSINESS("Kunde inte slå ihop utkast {} och {} i {} ({}): {}", a, b, label_of(year_), project_.folder.string(),
                   index.error());
      return pyroceph::tui::make_error_state(std::format("cratchit kunde inte slå ihop utkasten: {}", index.error()));
    }
    auto const one = merged(draft, other, event);
    LOG_BUSINESS("Utkast {} och {} ihopslagna i {} ({}) till {}: {} {} \"{}\", {} konteringar", a, b, label_of(year_),
                 project_.folder.string(), draft_id(*index, count - 1), format_iso_date(one.event.date),
                 format_amount(one.event.amount), one.event.heading, one.postings.size());
    report_(std::format("cratchit slog ihop {} och {} till {}: {}.", a, b, draft_id(*index, count - 1), draft_text(one)));
    done_ = true;
    return nullptr; // back to Samma? (back_to)
  }
  State const* back_to() const override { return done_ ? list_ : nullptr; }
  std::string help_topic() const override { return std::string(help_topic::same_pair); }

private:
  std::string number_;
  Project project_;
  FiscalYear year_;
  SameEvent pair_;
  std::shared_ptr<PairBooks const> books_;
  State const* list_;
  std::function<void(std::string)> report_;
  bool done_ = false;
};

// Samma?: the possible pairs of the year (find_same_events), numbered, the likely
// ones first, three lines each: the draft, the other draft or the voucher, and
// what the pair has going for it; a page at a time (< and >). A number and Enter
// opens the pair (PairState). Below the list, what was done here; report tells
// Utkast too. The pairs are found again when the user comes back.
class SameState : public State {
public:
  SameState(Project project, FiscalYear year, std::function<void(std::string)> report)
      : project_(std::move(project)), year_(std::move(year)), report_(std::move(report)) {
    read();
  }
  std::string name() const override { return "Samma?"; }
  View view() const override {
    if (!error_.empty()) return {std::format("cratchit kunde inte läsa bokföringen: {}", error_)};
    View lines;
    auto const window = std::format("samma belopp, högst {} dagar emellan", same_event_window_days);
    if (pairs_.empty()) {
      lines.push_back(std::format("Inga möjliga par i {} ({}).", label_of(year_), window));
    } else {
      lines.push_back(std::format("Möjliga par i {} ({}):", label_of(year_), window));
      auto const page = pager_.view();
      lines.insert(lines.end(), page.begin(), page.end());
    }
    lines.insert(lines.end(), done_.begin(), done_.end());
    return lines;
  }
  OptionsList options() const override { return {{back_key, "Föregående sida"}, {forward_key, "Nästa sida"}}; }
  bool on_key_in_place(CodePoint key) override {
    if (key == back_key) return pager_.back();
    if (key == forward_key) return pager_.forward();
    return false;
  }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  // A pair's number opens it
  InputResult on_input(std::u32string_view text) override {
    auto const typed = utf8_of(text);
    std::size_t number = 0;
    auto const [end, error] = std::from_chars(typed.data(), typed.data() + typed.size(), number);
    if (typed.empty() || error != std::errc{} || end != typed.data() + typed.size() || number == 0) {
      return std::unexpected(std::string("Skriv parets nummer, till exempel 1."));
    }
    if (number > pairs_.size()) return std::unexpected(std::format("Det finns inget par {}.", typed));
    return Accepted(std::make_unique<PairState>(typed, project_, year_, pairs_[number - 1], books_, this,
                                                [this](std::string line) {
                                                  done_.push_back(line);
                                                  report_(std::move(line));
                                                }));
  }
  void on_return() override { read(); }
  std::string help_topic() const override { return std::string(help_topic::same); }

private:
  void read() {
    error_.clear();
    pairs_.clear();
    auto const books = books::read_books(project_.folder);
    if (!books) {
      LOG_BUSINESS("Kunde inte läsa bokföringen i {}: {}", project_.folder.string(), books.error());
      error_ = books.error();
      pager_.set({});
      return;
    }
    auto const start = parse_iso_date(year_.start);
    auto const* year = start ? books->year_starting(*start) : nullptr;
    auto held = std::make_shared<PairBooks>();
    held->accounts = books->accounts;
    if (year != nullptr) {
      held->drafts = year->drafts;
      held->vouchers = year->vouchers;
      pairs_ = find_same_events(*year);
    }
    books_ = std::move(held);
    std::vector<std::string> lines;
    for (std::size_t i = 0; i < pairs_.size(); ++i) {
      auto const& pair = pairs_[i];
      auto const kind = pair.kind == SameEvent::Kind::drafts ? "utkast" : "bokfört";
      lines.push_back(std::format("  {:<3}{:<9}{}", i + 1, kind,
                                  unindented(draft_row(pair.draft, books_->drafts.size(), books_->drafts[pair.draft]))));
      auto const second = pair.kind == SameEvent::Kind::drafts ? draft_row(pair.other, books_->drafts.size(), books_->drafts[pair.other])
                                                                : voucher_row(books_->vouchers[pair.other]);
      lines.push_back(std::format("{:<14}{}", "", unindented(second)));
      lines.push_back(std::format("{:<14}{}", "", pair_note(pair)));
    }
    pager_.set(std::move(lines));
  }

  Project project_;
  FiscalYear year_;
  std::function<void(std::string)> report_;
  std::vector<SameEvent> pairs_;
  std::shared_ptr<PairBooks const> books_ = std::make_shared<PairBooks const>();
  pyroceph::tui::Pager pager_{same_page_pairs * 3};
  std::string error_;
  View done_; // what was done here, until the user leaves
};

// ASCII and the Latin-1 letters in UTF-8 to lower case, to find an account by its name
std::string lower_case(std::string_view text) {
  std::string result(text);
  for (std::size_t i = 0; i < result.size(); ++i) {
    auto const c = static_cast<unsigned char>(result[i]);
    if (c >= 'A' && c <= 'Z') {
      result[i] = static_cast<char>(c - 'A' + 'a');
    } else if (c == 0xC3 && i + 1 < result.size()) {
      auto const next = static_cast<unsigned char>(result[i + 1]);
      if (next >= 0x80 && next <= 0x9E && next != 0x97) result[i + 1] = static_cast<char>(next + 0x20);
      ++i;
    }
  }
  return result;
}

bool all_digits(std::string_view text) {
  return !text.empty() && std::ranges::all_of(text, [](char c) { return c >= '0' && c <= '9'; });
}

// The accounts whose number starts with word (digits), or whose name holds it (else, case
// folded), as "6570  Bankkostnader", at most max of them
std::vector<std::string> accounts_like(std::vector<books::Account> const& accounts, std::string_view word,
                                       std::size_t max = 20) {
  std::vector<std::string> found;
  auto const by_number = all_digits(word);
  auto const folded = lower_case(word);
  for (auto const& account : accounts) {
    auto const fits = by_number ? account.number.starts_with(word) : lower_case(account.name).contains(folded);
    if (!fits) continue;
    if (found.size() == max) {
      found.push_back("  …");
      break;
    }
    found.push_back(std::format("{}  {}", account.number, account.name));
  }
  return found;
}

// The words of a line, split at spaces
std::vector<std::string> words_of(std::string_view line) {
  std::vector<std::string> words;
  std::string word;
  for (char const c : line) {
    if (c == ' ') {
      if (!word.empty()) words.push_back(std::move(word));
      word.clear();
    } else {
      word += c;
    }
  }
  if (!word.empty()) words.push_back(std::move(word));
  return words;
}

// A posting typed by an account's name: the words of the name and the amount after
// them, the longest run of last words that reads as an amount (e.g. "1 118,00"), with
// at least one word left for the name
struct NamedPosting {
  std::vector<std::string> name;
  std::string amount; // as typed; empty when none
};

NamedPosting named_posting(std::string_view line) {
  auto const words = words_of(line);
  for (std::size_t at = 1; at < words.size(); ++at) {
    std::string amount;
    for (std::size_t i = at; i < words.size(); ++i) amount += (i == at ? "" : " ") + words[i];
    if (parse_typed_amount(amount)) return {{words.begin(), words.begin() + static_cast<std::ptrdiff_t>(at)}, amount};
  }
  return {words, {}};
}

// The accounts whose name holds every word (case folded, in any order)
std::vector<books::Account> accounts_named(std::vector<books::Account> const& accounts,
                                           std::vector<std::string> const& words) {
  std::vector<books::Account> found;
  for (auto const& account : accounts) {
    auto const name = lower_case(account.name);
    if (std::ranges::all_of(words, [&](std::string const& word) { return name.contains(lower_case(word)); })) {
      found.push_back(account);
    }
  }
  return found;
}

// Where the postings of a draft stand, for Kontera: what remains for them to balance,
// how debit stands against the amount, and whether it is ready
View posting_status(books::Draft const& draft) {
  View lines;
  auto const counted = std::ranges::count_if(draft.postings, [](books::Posting const& p) {
    return p.kind != books::Posting::Kind::removed;
  });
  auto const sum = books::sum(draft.postings);
  if (counted == 0) {
    lines.push_back("Inga konteringar än.");
  } else if (sum < 0) {
    lines.push_back(std::format("Kvar: debet {}.", format_amount(-sum)));
  } else if (sum > 0) {
    lines.push_back(std::format("Kvar: kredit {}.", format_amount(sum)));
  } else {
    lines.push_back("Konteringarna balanserar.");
  }
  auto const amount = draft.event.amount < 0 ? -draft.event.amount : draft.event.amount;
  if (amount != 0 && counted > 0 && books::debit(draft.postings) != amount) {
    lines.push_back(std::format("Debet {}, beloppet {}.", format_amount(books::debit(draft.postings)),
                                format_amount(amount)));
  }
  if (books::stage(draft) == books::DraftStage::ready) {
    lines.push_back("Utkastet är klart att bokföra: tryck - och sedan 2.");
  } else if (draft.event.heading.empty() || draft.event.amount == 0) {
    lines.push_back("Rubrik och belopp ändrar du i Händelse: tryck - och sedan 0.");
  }
  return lines;
}

// Shared by Kontera and Händelse: the draft being completed, where it is and how to change it
struct Completing {
  Project project;
  FiscalYear year;
  std::size_t index = 0;
  std::size_t count = 0; // the year's drafts, for the id (draft_id)
  books::Draft draft;
  std::vector<books::Account> accounts;

  // Writes changed in the place of the draft (replace_draft) and follows it
  std::expected<void, std::string> change(books::Draft changed, std::string_view what) {
    auto const year_start = parse_iso_date(year.start);
    if (!year_start) return std::unexpected(std::format("Felaktigt räkenskapsår {}", year.start));
    auto const at = replace_draft(project.folder, *year_start, index, draft, changed);
    if (!at) {
      LOG_BUSINESS("Kunde inte ändra utkast {} i {} ({}): {}", draft_id(index, count), label_of(year), project.folder.string(),
                   at.error());
      return std::unexpected(at.error());
    }
    LOG_BUSINESS("Utkast {} i {} ({}): {}; nu {}", draft_id(index, count), label_of(year), project.folder.string(),
                 what, draft_id(*at, count));
    index = *at;
    draft = std::move(changed);
    return {};
  }
};

// After a template (beslut 2026-11-18): the way on to Bokför, one missing part at
// a time. Each step, once done, goes back to origin (the draft's view or
// Fullborda) and from there on to the next (after_back), so - in any step
// leaves the chain at the draft. book makes Bokför for the draft as it is then.
struct Chain {
  State const* origin = nullptr;
  std::function<std::unique_ptr<State>()> book;
};

// The next step of the chain for the draft: Bokför when it is ready, else the
// heading, the amount, or Kontera for the postings, the first that is missing
std::unique_ptr<State> next_step(std::shared_ptr<Completing> const& completing, Chain const& chain);

// The part of a draft's event that EventPartState changes
enum class EventPart { date, heading, amount, series };

// Datum, Rubrik or Belopp (in Händelse): one part of the draft's event, typed in
// for the user to change: the date; the heading, else the text of the same event
// (same_text); the amount, else what the parts amount to. Enter changes it at
// once (replace_draft) and goes back to Händelse; what cannot be read, an empty
// heading, a zero amount or a date outside the year is refused with the reason,
// and the line kept.
class EventPartState : public State {
public:
  EventPartState(std::shared_ptr<Completing> completing, EventPart part, State const* event)
      : completing_(std::move(completing)), part_(part), event_(event) {}
  // A step of the chain: once done, on to the next (next_step) from the draft
  EventPartState(std::shared_ptr<Completing> completing, EventPart part, Chain chain)
      : completing_(std::move(completing)), part_(part), event_(chain.origin), chain_(std::move(chain)) {}
  std::string name() const override {
    switch (part_) {
    case EventPart::date: return "Datum";
    case EventPart::heading: return "Rubrik";
    case EventPart::amount: return "Belopp";
    case EventPart::series: return "Serie";
    }
    return {};
  }
  View view() const override {
    switch (part_) {
    case EventPart::date:
      return {std::format("Skriv datumet, till exempel 2026-10-22, och tryck Enter. Räkenskapsåret är {} – {}.",
                          completing_->year.start, completing_->year.end)};
    case EventPart::heading: return {"Skriv rubriken och tryck Enter."};
    case EventPart::amount:
      return {"Skriv beloppet och tryck Enter. Ett minustecken betyder att händelsen går åt andra hållet än vanligt."};
    case EventPart::series:
      return {"Skriv serien som utkastet ska bokföras i, till exempel D, och tryck Enter. Bokför börjar med den. "
              "En tom rad tar bort serien."};
    }
    return {};
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  std::u32string start_input() const override {
    auto const& draft = completing_->draft;
    switch (part_) {
    case EventPart::date: return code_points_of(format_iso_date(draft.event.date));
    case EventPart::heading: {
      if (!draft.event.heading.empty()) return code_points_of(draft.event.heading);
      if (auto const same = same_text(draft); !same.empty()) return code_points_of(same);
      // Else the text of a posting, e.g. from the statement entry
      auto const posting = std::ranges::find_if(draft.postings, [](books::Posting const& p) {
        return p.kind != books::Posting::Kind::removed && !p.text.empty();
      });
      return posting != draft.postings.end() ? code_points_of(posting->text) : std::u32string();
    }
    case EventPart::amount: {
      auto const amount = draft.event.amount != 0 ? draft.event.amount : amount_of(draft);
      return amount != 0 ? code_points_of(format_amount(amount)) : std::u32string();
    }
    case EventPart::series: return code_points_of(draft.series);
    }
    return {};
  }
  InputResult on_input(std::u32string_view text) override {
    auto typed = utf8_of(text);
    while (!typed.empty() && typed.back() == ' ') typed.pop_back();
    while (!typed.empty() && typed.front() == ' ') typed.erase(0, 1);
    auto changed = completing_->draft;
    std::string what;
    switch (part_) {
    case EventPart::date: {
      auto const date = typed.contains('-') ? parse_iso_date(typed) : parse_compact_date(typed);
      if (!date) return std::unexpected(std::format("{} är inget datum. Skriv till exempel 2026-10-22.", typed));
      changed.event.date = *date;
      what = "datum " + format_iso_date(*date);
      break;
    }
    case EventPart::heading:
      if (typed.empty()) return std::unexpected(std::string("Skriv rubriken."));
      changed.event.heading = typed;
      what = std::format("rubrik \"{}\"", typed);
      break;
    case EventPart::amount: {
      auto const amount = parse_typed_amount(typed);
      if (!amount) {
        return std::unexpected(std::format("{} är inget belopp. Skriv till exempel 1118,00 eller -1118,00.", typed));
      }
      if (*amount == 0) return std::unexpected(std::string("Beloppet är noll."));
      changed.event.amount = *amount;
      what = "belopp " + format_amount(*amount);
      break;
    }
    case EventPart::series: {
      for (auto& c : typed) {
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
      }
      if (!std::ranges::all_of(typed, [](char c) { return c >= 'A' && c <= 'Z'; })) {
        return std::unexpected(std::format("{} är ingen serie. Skriv seriens bokstav, till exempel D.", typed));
      }
      changed.series = typed;
      what = typed.empty() ? std::string("ingen serie") : "serie " + typed;
      break;
    }
    }
    if (auto done = completing_->change(std::move(changed), "händelsen ändrad: " + what); !done) {
      return std::unexpected(std::format("cratchit kunde inte ändra utkastet: {}", done.error()));
    }
    done_ = true;
    return Accepted(); // back to Händelse, or in the chain on to the next step
  }
  State const* back_to() const override { return done_ ? event_ : nullptr; }
  std::unique_ptr<State> after_back() override {
    return chain_ && done_ ? next_step(completing_, *chain_) : nullptr;
  }
  std::string help_topic() const override { return std::string(help_topic::complete_event); }

private:
  std::shared_ptr<Completing> completing_;
  EventPart part_;
  State const* event_;
  std::optional<Chain> chain_;
  bool done_ = false;
};

// Händelse (of a draft, beslut 2026-11-18): as a voucher's event, its date,
// heading and amount as options, but each leads to EventPartState to change it
// (a heading or amount the draft lacks shows empty). The options show the event
// as it is now. Opened with 0 in the draft's view and in Fullborda.
class DraftEventState : public State {
public:
  explicit DraftEventState(std::shared_ptr<Completing> completing) : completing_(std::move(completing)) {}
  std::string name() const override { return "Händelse"; }
  View view() const override { return {std::format("Händelsen i {}", draft_id(completing_->index, completing_->count))}; }
  OptionsList options() const override {
    auto const& event = completing_->draft.event;
    return {{U'0', "Datum: " + format_iso_date(event.date)},
            {U'1', event.heading.empty() ? std::string("Rubrik:") : "Rubrik: " + event.heading},
            {U'2', event.amount == 0 ? std::string("Belopp:") : "Belopp: " + format_amount(event.amount)},
            {U'3', completing_->draft.series.empty() ? std::string("Serie:") : "Serie: " + completing_->draft.series}};
  }
  OnKeyResult on_key(CodePoint key) override {
    switch (key) {
    case U'0': return std::make_unique<EventPartState>(completing_, EventPart::date, this);
    case U'1': return std::make_unique<EventPartState>(completing_, EventPart::heading, this);
    case U'2': return std::make_unique<EventPartState>(completing_, EventPart::amount, this);
    case U'3': return std::make_unique<EventPartState>(completing_, EventPart::series, this);
    default: return nullptr;
    }
  }
  std::string help_topic() const override { return std::string(help_topic::complete_event); }

private:
  std::shared_ptr<Completing> completing_;
};

// Kontera: the draft as its view shows it, with where its postings stand
// (posting_status). The user types a posting, an account and an amount (debit
// positive, credit negative, e.g. "6570 3,70"); the account alone takes what
// remains for the postings to balance. The account may also be typed as words
// of its name (beslut 2026-11-18): when exactly one account's name holds them
// all, Enter shows the line again with its number and the amount (as typed,
// else what remains) for the user to check and Enter once more; else the
// accounts that fit are listed. Tab completes an account number, or tells which
// accounts' names hold the words. Each posting is added at once
// (replace_draft) and the user stays here for the next one. Kontera works with
// the postings only: the event is changed in Händelse (0 in the draft's view and
// in Fullborda; beslut 2026-11-18). 9 Ta bort sista removes the last posting typed
// (one from a statement entry stays: Inte samma splits it off).
class PostingState : public State {
public:
  explicit PostingState(std::shared_ptr<Completing> completing) : completing_(std::move(completing)) {}
  // A step of the chain: once the draft is ready, on to Bokför from the draft
  PostingState(std::shared_ptr<Completing> completing, Chain chain)
      : completing_(std::move(completing)), chain_(std::move(chain)) {}
  std::string name() const override { return "Kontera"; }
  View view() const override {
    auto const& c = *completing_;
    auto lines = draft_view_lines(c.draft, draft_id(c.index, c.count), label_of(c.year), c.accounts);
    lines.push_back("");
    auto const status = posting_status(c.draft);
    lines.insert(lines.end(), status.begin(), status.end());
    // Where postings from statement entries came from, e.g. "1930 från bank/nordea.csv rad 2"
    for (auto const& posting : c.draft.postings) {
      if (posting.source.empty()) continue;
      auto const colon = posting.source.rfind(':');
      lines.push_back(colon == std::string::npos
                          ? std::format("{} från {}", posting.account, posting.source)
                          : std::format("{} från {} rad {}", posting.account, posting.source.substr(0, colon),
                                        posting.source.substr(colon + 1)));
    }
    if (!error_.empty()) lines.push_back(error_);
    return lines;
  }
  // Only the postings: the event is changed in Händelse, where the whole draft is shown
  OptionsList options() const override {
    if (removable()) return {{U'9', "Ta bort sista"}};
    return {};
  }
  bool on_key_in_place(CodePoint key) override {
    if (key != U'9' || !removable()) return false;
    auto changed = completing_->draft;
    auto const removed = changed.postings.back();
    changed.postings.pop_back();
    auto const what = std::format("kontering borttagen: {} {}", removed.account, format_amount(removed.amount));
    if (auto done = completing_->change(std::move(changed), what); !done) {
      error_ = std::format("cratchit kunde inte ändra utkastet: {}", done.error());
    } else {
      error_.clear();
    }
    return true;
  }
  OnKeyResult on_key(CodePoint) override {
    return nullptr;
  }
  Completion complete(std::u32string_view text) const override {
    auto const typed = utf8_of(text);
    auto const words = words_of(typed);
    if (words.empty()) return {};
    Completion completion;
    if (all_digits(words.front())) {
      if (words.size() > 1 || typed.ends_with(' ')) return {};
      auto const found = accounts_like(completing_->accounts, typed);
      if (found.size() == 1) {
        auto const number = found.front().substr(0, found.front().find(' '));
        completion.added = u32_of(number.substr(typed.size()) + " ");
        completion.notes.push_back(found.front());
      } else {
        completion.candidates = found;
      }
      return completion;
    }
    auto const found = accounts_named(completing_->accounts, named_posting(typed).name);
    if (found.empty()) {
      completion.notes.push_back("Inget konto har det i namnet.");
    } else if (found.size() == 1) {
      completion.notes.push_back(std::format("{}  {} passar; Enter väljer det.", found.front().number, found.front().name));
    } else {
      for (std::size_t i = 0; i < found.size() && i < 20; ++i) {
        completion.notes.push_back(std::format("{}  {}", found[i].number, found[i].name));
      }
      if (found.size() > 20) completion.notes.push_back("  …");
    }
    return completion;
  }
  // A line chosen by name, with its number, for the user to check (see on_input)
  std::u32string start_input() const override { return pending_; }
  InputResult on_input(std::u32string_view text) override {
    pending_.clear();
    auto const typed = utf8_of(text);
    auto const space = typed.find(' ');
    auto const account = typed.substr(0, space);
    auto const rest = space == std::string::npos ? std::string() : typed.substr(space + 1);
    auto& c = *completing_;
    if (account.empty()) return std::unexpected(std::string("Skriv konto och belopp, till exempel 6570 3,70."));
    if (!all_digits(account)) {
      // Words of a name: one account that fits shows the line again, with its number
      auto const named = named_posting(typed);
      auto const found = accounts_named(c.accounts, named.name);
      std::string words;
      for (auto const& word : named.name) words += (words.empty() ? "" : " ") + word;
      if (found.empty()) return std::unexpected(std::format("Inget konto har {} i namnet.", words));
      if (found.size() > 1) {
        std::string error = "Flera konton passar:";
        for (std::size_t i = 0; i < found.size() && i < 10; ++i) {
          error += std::format("\n  {}  {}", found[i].number, found[i].name);
        }
        if (found.size() > 10) error += "\n  …";
        return std::unexpected(error + "\nSkriv fler ord eller numret.");
      }
      auto const remains = -books::sum(c.draft.postings);
      auto const amount = !named.amount.empty() ? named.amount : remains != 0 ? format_amount(remains) : std::string();
      pending_ = code_points_of(std::format("{} {}", found.front().number, amount));
      return Accepted(std::format("{}  {} passar. Kontrollera beloppet och tryck Enter.", found.front().number,
                                  found.front().name));
    }
    if (std::ranges::find(c.accounts, account, &books::Account::number) == c.accounts.end()) {
      auto const found = accounts_like(c.accounts, account, 10);
      std::string error = std::format("Kontot {} finns inte i kontoplanen.", account);
      if (!found.empty()) {
        error += " Konton som passar:";
        for (auto const& line : found) error += "\n  " + line;
      }
      return std::unexpected(error);
    }
    Amount amount = 0;
    if (rest.find_first_not_of(' ') == std::string::npos) {
      amount = -books::sum(c.draft.postings);
      if (amount == 0) return std::unexpected(std::string("Skriv beloppet: debet positivt, kredit negativt, till exempel -3,70."));
    } else {
      auto const parsed = parse_typed_amount(rest);
      if (!parsed) return std::unexpected(std::format("{} är inget belopp. Skriv till exempel 3,70 eller -3,70.", rest));
      if (*parsed == 0) return std::unexpected(std::string("Beloppet är noll."));
      amount = *parsed;
    }
    auto changed = c.draft;
    books::Posting posting;
    posting.account = account;
    posting.amount = amount;
    changed.postings.push_back(std::move(posting));
    if (auto done = c.change(std::move(changed), std::format("kontering tillagd: {} {}", account, format_amount(amount)));
        !done) {
      return std::unexpected(std::format("cratchit kunde inte ändra utkastet: {}", done.error()));
    }
    error_.clear();
    if (chain_ && books::stage(c.draft) == books::DraftStage::ready) ready_ = true;
    return Accepted();
  }
  State const* back_to() const override { return ready_ ? chain_->origin : nullptr; }
  std::unique_ptr<State> after_back() override { return ready_ ? chain_->book() : nullptr; }
  std::string help_topic() const override { return std::string(help_topic::post); }

private:
  // The last posting was typed (not from a statement entry)
  bool removable() const {
    auto const& postings = completing_->draft.postings;
    return !postings.empty() && postings.back().source.empty() &&
           postings.back().kind != books::Posting::Kind::removed;
  }

  std::shared_ptr<Completing> completing_;
  std::optional<Chain> chain_;
  bool ready_ = false; // in the chain: the draft became ready
  std::string error_;
  std::u32string pending_; // a line chosen by name, to check and Enter again
};

std::unique_ptr<State> next_step(std::shared_ptr<Completing> const& completing, Chain const& chain) {
  auto const& draft = completing->draft;
  if (books::stage(draft) == books::DraftStage::ready) return chain.book();
  if (draft.event.heading.empty()) return std::make_unique<EventPartState>(completing, EventPart::heading, chain);
  if (draft.event.amount == 0) return std::make_unique<EventPartState>(completing, EventPart::amount, chain);
  return std::make_unique<PostingState>(completing, chain);
}

// Mall (3 Använd mall, beslut 2026-11-18): earlier vouchers like the draft
// (find_templates), numbered, the best first, each with its rows. A number and
// Enter uses it at once (apply_template; the template shown in full is the
// confirmation, and what it adds is removed with 9 Ta bort sista in Kontera):
// the draft gets the rows and the template's series, not its text. The reply
// tells what was added and which accounts were left out; then on through the
// chain (next_step): what is missing, one part at a time, and Bokför.
class TemplateState : public State {
public:
  TemplateState(std::shared_ptr<Completing> completing, Chain chain)
      : completing_(std::move(completing)), chain_(std::move(chain)) {
    auto const books = books::read_books(completing_->project.folder);
    if (!books) {
      error_ = books.error();
      return;
    }
    templates_ = find_templates(*books, completing_->draft);
  }
  std::string name() const override { return "Mall"; }
  View view() const override {
    if (!error_.empty()) return {std::format("cratchit kunde inte läsa bokföringen: {}", error_)};
    auto const& c = *completing_;
    auto const own = std::ranges::any_of(c.draft.postings, [](books::Posting const& p) {
      return p.kind != books::Posting::Kind::removed;
    });
    auto const how = own ? "med samma konton och samma tecken som utkastets konteringar"
                         : "vars text delar ord med utkastets rubrik";
    View lines{"Utkastet:", draft_row(c.index, c.count, c.draft)};
    if (templates_.empty()) {
      lines.push_back(std::format("Inga tidigare verifikationer {}.", how));
      return lines;
    }
    lines.push_back(std::format("Tidigare verifikationer {}:", how));
    for (std::size_t i = 0; i < templates_.size(); ++i) {
      auto const& voucher = templates_[i].voucher;
      lines.push_back(std::format("  {:<3}{}", i + 1, unindented(voucher_row(voucher))));
      std::size_t name_width = 0;
      for (auto const& posting : voucher.postings) {
        name_width = std::max(name_width, width_of(account_name(c.accounts, posting.account)));
      }
      for (auto const& posting : voucher.postings) {
        if (posting.kind == books::Posting::Kind::removed) continue;
        auto name = account_name(c.accounts, posting.account);
        for (auto shown = width_of(name); shown < name_width; ++shown) name += ' ';
        lines.push_back(std::format("       {}  {}  {:>12}", posting.account, name, format_amount(posting.amount)));
      }
    }
    lines.push_back("Skriv mallens nummer och tryck Enter.");
    return lines;
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  InputResult on_input(std::u32string_view text) override {
    auto const typed = utf8_of(text);
    std::size_t number = 0;
    auto const [end, error] = std::from_chars(typed.data(), typed.data() + typed.size(), number);
    if (typed.empty() || error != std::errc{} || end != typed.data() + typed.size() || number == 0) {
      return std::unexpected(std::string("Skriv mallens nummer, till exempel 1."));
    }
    if (number > templates_.size()) return std::unexpected(std::format("Det finns ingen mall {}.", typed));
    auto& c = *completing_;
    auto const& voucher = templates_[number - 1].voucher;
    auto const id = books::voucher_id(voucher.series, voucher.number);
    auto const before = c.draft.postings.size();
    auto applied = apply_template(c.draft, voucher, c.accounts);
    auto const added = applied.draft.postings.size() - before;
    if (auto done = c.change(std::move(applied.draft), std::format("mall {} använd, {} rader tillagda", id, added));
        !done) {
      return std::unexpected(std::format("cratchit kunde inte ändra utkastet: {}", done.error()));
    }
    auto reply = std::format("cratchit använde {} som mall: {} {}, serie {}.", id, added,
                             added == 1 ? "rad tillagd" : "rader tillagda", voucher.series);
    if (!applied.left_out.empty()) {
      reply += " Kontona finns inte i kontoplanen och kom inte med:";
      for (auto const& account : applied.left_out) reply += " " + account;
      reply += ".";
    }
    done_ = true;
    return Accepted(reply); // back to the draft, and on through the chain
  }
  State const* back_to() const override { return done_ ? chain_.origin : nullptr; }
  std::unique_ptr<State> after_back() override { return done_ ? next_step(completing_, chain_) : nullptr; }
  std::string help_topic() const override { return std::string(help_topic::use_template); }

private:
  std::shared_ptr<Completing> completing_;
  Chain chain_;
  std::vector<Template> templates_;
  std::string error_;
  bool done_ = false;
};

// Bokför: the voucher a ready draft is to become (book_draft), in full: date,
// text, amount, the day it is registered and the postings, then the books'
// series, each with its name and the number its next voucher gets. The user
// types a series and Enter, which books it (the voucher shown in full is the
// confirmation; beslut 2026-11-18): reported through report, then back to
// Fullborda (back_to). A series that is not the books' is refused.
class BookState : public State {
public:
  BookState(Project project, FiscalYear year, std::size_t index, std::size_t count, books::Draft draft,
            State const* complete, std::function<void(std::string)> report)
      : project_(std::move(project)), year_(std::move(year)), index_(index), count_(count), draft_(std::move(draft)),
        complete_(complete), report_(std::move(report)) {
    read();
  }
  std::string name() const override { return "Bokför"; }
  View view() const override {
    if (!error_.empty()) return {std::format("cratchit kunde inte läsa bokföringen: {}", error_)};
    auto const row = [](std::string_view label, std::string const& value) {
      return std::format("  {:<16}{}", std::string(label) + ":", value);
    };
    auto const amount = draft_.event.amount < 0 ? -draft_.event.amount : draft_.event.amount;
    View lines{std::format("Bokför utkast {} som verifikation i {}:", draft_id(index_, count_), label_of(year_)),
               row("Datum", format_iso_date(draft_.event.date)), row("Text", draft_.event.heading),
               row("Belopp", format_amount(amount)), row("Registreras", format_iso_date(today_date())),
               "Konteringar:"};
    std::size_t account_width = 0;
    std::size_t name_width = 0;
    for (auto const& posting : draft_.postings) {
      account_width = std::max(account_width, width_of(posting.account));
      name_width = std::max(name_width, width_of(account_name(accounts_, posting.account)));
    }
    auto const padded = [](std::string text, std::size_t width) {
      for (auto shown = width_of(text); shown < width; ++shown) text += ' ';
      return text;
    };
    for (auto const& posting : draft_.postings) {
      if (posting.kind == books::Posting::Kind::removed) continue;
      auto line = std::format("  {}  {}  {:>12}", padded(posting.account, account_width),
                              padded(account_name(accounts_, posting.account), name_width),
                              format_amount(posting.amount));
      if (!posting.text.empty()) line += "  " + posting.text;
      if (posting.date && *posting.date != draft_.event.date) line += "  " + format_iso_date(*posting.date);
      lines.push_back(std::move(line));
    }
    lines.push_back("Serier:");
    for (auto const& series : series_) lines.push_back("  " + series);
    lines.push_back("Skriv serien och tryck Enter för att bokföra. Verifikationen går inte att ändra efteråt.");
    return lines;
  }
  OptionsList options() const override { return {}; }
  OnKeyResult on_key(CodePoint) override { return nullptr; }
  // The draft's series (from its template, or chosen in Händelse), to keep or change
  std::u32string start_input() const override { return code_points_of(draft_.series); }
  InputResult on_input(std::u32string_view text) override {
    auto series = utf8_of(text);
    while (!series.empty() && series.back() == ' ') series.pop_back();
    while (!series.empty() && series.front() == ' ') series.erase(0, 1);
    for (auto& c : series) {
      if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    }
    if (series.empty()) return std::unexpected(std::string("Skriv serien, till exempel A."));
    auto const year_start = parse_iso_date(year_.start);
    if (!year_start) return std::unexpected(std::format("Felaktigt räkenskapsår {}", year_.start));
    auto const voucher = book_draft(project_.folder, *year_start, index_, draft_, series, today_date());
    if (!voucher) {
      LOG_BUSINESS("Kunde inte bokföra utkast {} i {} ({}): {}", draft_id(index_, count_), label_of(year_),
                   project_.folder.string(), voucher.error());
      return std::unexpected(voucher.error());
    }
    auto const id = books::voucher_id(voucher->series, voucher->number);
    LOG_BUSINESS("Utkast {} bokfört i {} ({}) som {}: {} {} \"{}\", registrerad {}", draft_id(index_, count_), label_of(year_),
                 project_.folder.string(), id, format_iso_date(voucher->event.date),
                 format_amount(voucher->event.amount), voucher->event.heading,
                 format_iso_date(voucher->registered.value_or(today_date())));
    for (auto const& posting : voucher->postings) {
      LOG_BUSINESS("  kontering: {} {} {} \"{}\"", posting.account, format_amount(posting.amount),
                   posting.date ? format_iso_date(*posting.date) : std::string(), posting.text);
    }
    auto reply = std::format("cratchit bokförde {} som {}: {}.", draft_id(index_, count_), id, voucher->event.heading);
    report_(reply);
    done_ = true;
    return Accepted(reply); // shown once in the terminal, then back to Fullborda
  }
  State const* back_to() const override { return done_ ? complete_ : nullptr; }
  std::string help_topic() const override { return std::string(help_topic::book); }

private:
  void read() {
    auto const books = books::read_books(project_.folder);
    if (!books) {
      error_ = books.error();
      return;
    }
    accounts_ = books->accounts;
    auto const start = parse_iso_date(year_.start);
    auto const* year = start ? books->year_starting(*start) : nullptr;
    for (auto const& series : books->series) {
      auto const next = year != nullptr ? next_number(*year, series.series) : std::string("1");
      series_.push_back(series.name.empty() ? std::format("{}  (nästa {}{})", series.series, series.series, next)
                                            : std::format("{}  {} (nästa {}{})", series.series, series.name,
                                                          series.series, next));
    }
  }

  Project project_;
  FiscalYear year_;
  std::size_t index_;
  std::size_t count_;
  books::Draft draft_;
  State const* complete_;
  std::function<void(std::string)> report_;
  std::vector<books::Account> accounts_;
  std::vector<std::string> series_;
  std::string error_;
  bool done_ = false;
};

// Fullborda #<n>: the drafts one at a time, from the oldest (date order), each as
// its view shows it, with "#n av N" in the heading. < and > go to the one before
// and after (in place; at either end they do nothing). The options are those of
// the draft's view (beslut 2026-11-18: the same options wherever a draft is
// shown): 0 Händelse and 1 Konteringar (Kontera), 2 Bokför for a draft that is
// ready, 9 Glöm or Inte samma. The view is the draft and its options; what was
// done is told to Utkast (Bokför also replies once in the terminal). The drafts are read again when the user comes back, at the same place.
class CompleteState : public State {
public:
  CompleteState(Project project, FiscalYear year, std::function<void(std::string)> report)
      : project_(std::move(project)), year_(std::move(year)), report_(std::move(report)) {
    read();
  }
  std::string name() const override {
    return drafts_.empty() ? std::string("Fullborda") : "Fullborda " + draft_id(index_, drafts_.size());
  }
  View view() const override {
    if (!error_.empty()) return {std::format("cratchit kunde inte läsa bokföringen: {}", error_)};
    View lines;
    if (!failed_.empty()) lines.push_back(failed_);
    if (drafts_.empty()) {
      lines.push_back(std::format("Inga utkast att fullborda i {}.", label_of(year_)));
    } else {
      auto const shown = draft_view_lines(drafts_[index_], std::format("{} av {}", draft_id(index_, drafts_.size()), drafts_.size()),
                                          label_of(year_), accounts_);
      lines.insert(lines.end(), shown.begin(), shown.end());
      auto const missing = missing_to_book(drafts_[index_]);
      lines.insert(lines.end(), missing.begin(), missing.end());
    }
    return lines;
  }
  // As the draft's view (make_draft_view), with < and > to browse
  OptionsList options() const override {
    if (drafts_.empty()) return {};
    auto const& draft = drafts_[index_];
    OptionsList options{{U'0', "Händelse"}, {U'1', "Konteringar"}};
    if (books::stage(draft) == books::DraftStage::ready) options.push_back({U'2', "Bokför"});
    options.push_back({U'3', "Använd mall"});
    if (removal_of(draft) == DraftRemoval::forget) options.push_back({U'9', "Glöm"});
    if (removal_of(draft) == DraftRemoval::split) options.push_back({U'9', "Inte samma"});
    options.push_back({back_key, "Föregående"});
    options.push_back({forward_key, "Nästa"});
    return options;
  }
  bool on_key_in_place(CodePoint key) override {
    failed_.clear();
    if (key == back_key && index_ > 0) {
      --index_;
      return true;
    }
    if (key == forward_key && index_ + 1 < drafts_.size()) {
      ++index_;
      return true;
    }
    // Inte samma splits it at once (it can be undone in Samma?); the parts are read in their places
    if (key == U'9' && !drafts_.empty() && removal_of(drafts_[index_]) == DraftRemoval::split) {
      auto const count = split_logged(project_, year_, index_, drafts_.size(), drafts_[index_]);
      if (!count) {
        failed_ = std::format("cratchit kunde inte dela utkastet: {}", count.error());
      } else {
        report(std::format("cratchit delade {} i {} utkast.", draft_id(index_, drafts_.size()), *count));
      }
      read();
      return true;
    }
    return false; // at either end: nothing
  }
  OnKeyResult on_key(CodePoint key) override {
    failed_.clear();
    if (drafts_.empty()) return nullptr;
    auto const& draft = drafts_[index_];
    auto const completing = [&] {
      return std::make_shared<Completing>(Completing{project_, year_, index_, drafts_.size(), draft, accounts_});
    };
    switch (key) {
    case U'0': return std::make_unique<DraftEventState>(completing());
    case U'1': return std::make_unique<PostingState>(completing());
    case U'2':
      if (books::stage(draft) != books::DraftStage::ready) return nullptr;
      return std::make_unique<BookState>(project_, year_, index_, drafts_.size(), draft, this,
                                         [this](std::string line) { report(std::move(line)); });
    case U'3': {
      auto shared = completing();
      auto book = [this, shared]() -> std::unique_ptr<State> {
        auto const& c = *shared;
        return std::make_unique<BookState>(project_, year_, c.index, c.count, c.draft, this,
                                           [this](std::string line) { report(std::move(line)); });
      };
      return std::make_unique<TemplateState>(shared, Chain{this, std::move(book)});
    }
    case U'9': {
      if (removal_of(draft) != DraftRemoval::forget) return nullptr;
      auto summary = draft_row(index_, drafts_.size(), draft);
      summary = summary.substr(summary.find(format_iso_date(draft.event.date)));
      auto forget = [this, index = index_, count = drafts_.size(), draft]() -> std::expected<void, std::string> {
        if (auto forgotten = forget_logged(project_, year_, index, count, draft, "glömt"); !forgotten) return forgotten;
        report(std::format("cratchit glömde utkast {}: {}.", draft_id(index, count), draft_text(draft)));
        return {};
      };
      return make_forget_draft_state(draft_id(index_, drafts_.size()), summary, std::move(forget), this, this);
    }
    default: return nullptr;
    }
  }
  void on_return() override { read(); }
  std::string help_topic() const override { return std::string(help_topic::complete); }

private:
  // What was done here: told to Utkast
  void report(std::string line) {
    report_(std::move(line));
  }

  void read() {
    error_.clear();
    drafts_.clear();
    auto const books = books::read_books(project_.folder);
    if (!books) {
      LOG_BUSINESS("Kunde inte läsa bokföringen i {}: {}", project_.folder.string(), books.error());
      error_ = books.error();
      return;
    }
    accounts_ = books->accounts;
    auto const start = parse_iso_date(year_.start);
    if (auto const* year = start ? books->year_starting(*start) : nullptr; year != nullptr) drafts_ = year->drafts;
    if (index_ >= drafts_.size()) index_ = drafts_.empty() ? 0 : drafts_.size() - 1;
  }

  Project project_;
  FiscalYear year_;
  std::function<void(std::string)> report_;
  std::vector<books::Draft> drafts_;
  std::vector<books::Account> accounts_;
  std::size_t index_ = 0;
  std::string error_;
  std::string failed_; // a split that failed, until the next key
};

class DraftState : public State {
public:
  DraftState(Project project, FiscalYear year) : project_(std::move(project)), year_(std::move(year)) { read(); }
  std::string name() const override { return "Utkast"; }
  View view() const override {
    if (!error_.empty()) return {std::format("cratchit kunde inte läsa bokföringen: {}", error_)};
    View lines;
    if (pager_.count() == 0) {
      lines.push_back(std::format("Utkast {}: inga utkast.", label_of(year_)));
    } else {
      lines.push_back(std::format("Utkast {}: utkast {}–{} av {}", label_of(year_), pager_.first() + 1,
                                  pager_.first() + pager_.shown(), pager_.count()));
      auto const page = pager_.view();
      lines.insert(lines.end(), page.begin(), page.end());
    }
    lines.insert(lines.end(), report_.begin(), report_.end());
    return lines;
  }
  // < and > always, so that turning past either end does nothing (and starts no input)
  OptionsList options() const override {
    return {{U'0', "Beskriv händelser"},
            {U'1', "Samma?"},
            {U'2', "Fullborda utkast"},
            {U'3', "Läs kontoutdrag"},
            {U'4', "Läs pdf-underlag"},
            {U'5', "Gör bokfört till utkast"},
            {back_key, "Föregående sida"},
            {forward_key, "Nästa sida"}};
  }
  // < and > turn the page in place (keys match exactly, beslut.md)
  bool on_key_in_place(CodePoint key) override {
    if (key == back_key) return pager_.back();
    if (key == forward_key) return pager_.forward();
    return false;
  }
  OnKeyResult on_key(CodePoint key) override {
    if (key == U'1') {
      report_.clear();
      return std::make_unique<SameState>(project_, year_, [this](std::string line) { report_.push_back(std::move(line)); });
    }
    if (key == U'2') {
      report_.clear();
      return std::make_unique<CompleteState>(project_, year_,
                                             [this](std::string line) { report_.push_back(std::move(line)); });
    }
    if (key == U'5') {
      report_.clear();
      return make_unbook_menu(project_, year_, this, [this](std::string line) { report_.push_back(std::move(line)); });
    }
    if (key == U'4') {
      report_.clear();
      return std::make_unique<ReadPdfState>(project_, year_, folder_,
                                            [this](fs::path const& folder) { folder_ = folder; });
    }
    if (key == U'0') {
      report_.clear();
      return std::make_unique<DescribeEventState>(project_, year_, [this](View text) { report_ = std::move(text); });
    }
    if (key != U'3') return nullptr;
    report_.clear();
    return make_read_statement_menu(
        project_, year_, this,
        [this](View text, fs::path const& file) {
          report_ = std::move(text);
          folder_ = file.parent_path();
        },
        folder_);
  }
  // A draft's id as one word, e.g. #3, opens it (its place in the list, newest first: draft_id)
  InputResult on_input(std::u32string_view text) override {
    auto const typed = utf8_of(text);
    std::size_t number = 0;
    auto const digits = std::string_view{typed}.substr(typed.starts_with('#') ? 1 : 0);
    auto const [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), number);
    if (!typed.starts_with('#') || digits.empty() || error != std::errc{} || end != digits.data() + digits.size() ||
        number == 0) {
      return std::unexpected(std::string("Skriv utkastets id som #13."));
    }
    auto const books = books::read_books(project_.folder);
    if (!books) return std::unexpected(std::format("cratchit kunde inte läsa bokföringen: {}", books.error()));
    auto const start = parse_iso_date(year_.start);
    auto const* year = start ? books->year_starting(*start) : nullptr;
    auto const found = year != nullptr ? draft_index(number, year->drafts.size()) : std::nullopt;
    if (!found) return std::unexpected(std::format("Det finns inget utkast {} i {}.", typed, label_of(year_)));
    report_.clear();
    auto const index = *found;
    auto const count = year->drafts.size();
    auto completing =
        std::make_shared<Completing>(Completing{project_, year_, index, count, year->drafts[index], books->accounts});
    // The draft as the list shows it, without the id: date, amount and text
    auto const summary_of = [](std::size_t at, std::size_t of, books::Draft const& draft) {
      auto summary = draft_row(at, of, draft);
      return summary.substr(summary.find(format_iso_date(draft.event.date)));
    };
    DraftActions actions;
    actions.forget = [this, completing]() -> std::expected<void, std::string> {
      auto const& c = *completing;
      if (auto forgotten = forget_logged(project_, year_, c.index, c.count, c.draft, "glömt"); !forgotten) return forgotten;
      report_ = {std::format("cratchit glömde utkast {}: {}.", draft_id(c.index, c.count), draft_text(c.draft))};
      return {};
    };
    actions.split = [this, completing]() -> std::expected<void, std::string> {
      auto const& c = *completing;
      auto const count = split_logged(project_, year_, c.index, c.count, c.draft);
      if (!count) return std::unexpected(count.error());
      report_ = {std::format("cratchit delade {} i {} utkast.", draft_id(c.index, c.count), *count)};
      return {};
    };
    actions.book = [this, completing](State const*) -> std::unique_ptr<State> {
      auto const& c = *completing;
      // Booked, the draft is gone: back to Utkast, which tells it
      return std::make_unique<BookState>(project_, year_, c.index, c.count, c.draft, this,
                                         [this](std::string line) { report_ = {std::move(line)}; });
    };
    actions.edit_event = [completing](State const*) -> std::unique_ptr<State> {
      return std::make_unique<DraftEventState>(completing);
    };
    actions.edit_postings = [completing]() -> std::unique_ptr<State> { return std::make_unique<PostingState>(completing); };
    actions.use_template = [this, completing](State const* view) -> std::unique_ptr<State> {
      // Booked, the draft is gone: back to Utkast, as with 2 Bokför
      auto book = [this, completing]() -> std::unique_ptr<State> {
        auto const& c = *completing;
        return std::make_unique<BookState>(project_, year_, c.index, c.count, c.draft, this,
                                           [this](std::string line) { report_ = {std::move(line)}; });
      };
      return std::make_unique<TemplateState>(completing, Chain{view, std::move(book)});
    };
    actions.now = [completing, summary_of]() -> std::optional<DraftNow> {
      auto const& c = *completing;
      return DraftNow{c.draft, draft_id(c.index, c.count), summary_of(c.index, c.count, c.draft)};
    };
    auto const& draft = completing->draft;
    return Accepted(make_draft_view(draft, draft_id(index, count), label_of(year_), books->accounts, summary_of(index, count, draft),
                                    this, std::move(actions)));
  }
  void on_return() override { read(); }
  std::string help_topic() const override { return std::string(help_topic::drafts); }

private:
  void read() {
    error_.clear();
    auto const books = books::read_books(project_.folder);
    if (!books) {
      LOG_BUSINESS("Kunde inte läsa bokföringen i {}: {}", project_.folder.string(), books.error());
      error_ = books.error();
      pager_.set({});
      return;
    }
    auto const start = parse_iso_date(year_.start);
    pager_.set(draft_rows(start ? books->year_starting(*start) : nullptr));
  }

  Project project_;
  FiscalYear year_;
  pyroceph::tui::Pager pager_{booked_page_rows};
  std::string error_;
  View report_; // what was just done, shown until the user goes on from here
  std::optional<fs::path> folder_; // of the statement or pdf files read last: they often lie together
};

} // namespace

std::unique_ptr<State> make_draft_menu(Project project, FiscalYear year) {
  return std::make_unique<DraftState>(std::move(project), std::move(year));
}

std::unique_ptr<State> make_read_statement_menu(Project project, FiscalYear year, State const* origin,
                                                StatementReport report, std::optional<fs::path> start_folder) {
  return std::make_unique<ReadStatementState>(std::move(project), std::move(year), origin, std::move(report),
                                              std::move(start_folder));
}

std::unique_ptr<State> make_book_draft_state(Project project, FiscalYear year, std::size_t index, std::size_t count,
                                             books::Draft draft, State const* back,
                                             std::function<void(std::string)> report) {
  return std::make_unique<BookState>(std::move(project), std::move(year), index, count, std::move(draft), back,
                                     std::move(report));
}

} // namespace cratchit::ui
