#include "core/ar/notes.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <optional>

namespace cratchit::ar {

namespace {

// The row texts of a section: opening, disposal, other changes, closing
struct SectionTexts {
  std::string_view section;
  std::string_view opening;
  std::string_view disposal;
  std::string_view other;
  std::string_view closing;
};

constexpr std::array<SectionTexts, 3> section_texts{{
    {"Anskaffningsvärden", "Ingående anskaffningsvärden", "Försäljningar och utrangeringar", "Inköp",
     "Utgående anskaffningsvärden"},
    {"Avskrivningar", "Ingående avskrivningar", "Återförda avskrivningar", "Årets avskrivningar",
     "Utgående avskrivningar"},
    {"Nedskrivningar", "Ingående nedskrivningar", "Återförda nedskrivningar", "Årets nedskrivningar",
     "Utgående nedskrivningar"},
}};

// The changes are listed in this order: Inköp before Försäljningar, Återförda before Årets
bool disposal_first(std::string_view section) {
  return section != "Anskaffningsvärden";
}

bool is_disposal_result(std::string_view account) {
  return account.size() == 4 && (account.starts_with("397") || account.starts_with("797"));
}

// The post of the balansräkning a note title "Not <n> <post>" is about
std::optional<std::string_view> post_of(std::string_view title) {
  if (!title.starts_with("Not ")) {
    return std::nullopt;
  }
  auto const space = title.find(' ', 4);
  return space == std::string_view::npos ? std::nullopt : std::optional(title.substr(space + 1));
}

} // namespace

Notes build_notes(Statement const& statement, sie::Document const& document) {
  Notes notes;
  auto const& lines = statement.lines;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    auto const& heading = lines[index];
    if (!heading.in_notes || heading.kind != LineKind::heading || heading.level == 1 || !heading.shown) {
      continue;
    }
    Note note{heading.text, {}, "", 0};
    std::size_t at = index + 1;
    for (; at < lines.size() && lines[at].kind == LineKind::item; ++at) {
      note.sections.push_back({lines[at].text, lines[at].account_set, {}});
    }
    if (at < lines.size() && lines[at].kind == LineKind::sum) {
      note.sum_text = lines[at].text;
    }
    auto const in_note = [&note](std::string_view account) {
      return std::ranges::any_of(note.sections, [account](NoteSection const& section) { return section.accounts.contains(account); });
    };
    for (auto& section : note.sections) {
      auto const texts = std::ranges::find(section_texts, section.text, &SectionTexts::section);
      if (texts == section_texts.end()) {
        notes.errors.push_back(std::format("{}: unknown section {}; expected Anskaffningsvärden, Avskrivningar or Nedskrivningar",
                                           note.title, section.text));
        continue;
      }
      NoteRow opening{std::string(texts->opening), 0, {}, {}};
      NoteRow closing{std::string(texts->closing), 0, {}, {}};
      for (auto const& balance : document.opening) {
        if (balance.year == 0 && section.accounts.contains(balance.account)) {
          opening.amount += balance.amount;
          opening.accounts[balance.account] += balance.amount;
        }
      }
      for (auto const& balance : document.closing) {
        if (balance.year == 0 && section.accounts.contains(balance.account)) {
          closing.amount += balance.amount;
          closing.accounts[balance.account] += balance.amount;
        }
      }
      NoteRow disposal{std::string(texts->disposal), 0, {}, {}};
      NoteRow other{std::string(texts->other), 0, {}, {}};
      for (auto const& voucher : document.vouchers) {
        Amount here = 0;
        bool touches = false;
        bool disposes = false;
        for (auto const& transaction : voucher.transactions) {
          if (transaction.kind == sie::Transaction::Kind::removed) {
            continue;
          }
          if (section.accounts.contains(transaction.account)) {
            here += transaction.amount;
            touches = true;
          } else if (in_note(transaction.account) || is_disposal_result(transaction.account)) {
            disposes = true;
          }
        }
        if (touches) {
          auto& row = disposes ? disposal : other;
          row.amount += here;
          row.vouchers.push_back(&voucher);
        }
      }
      for (auto* row : {&disposal, &other}) {
        std::ranges::stable_sort(row->vouchers, [](auto const* left, auto const* right) { return left->date < right->date; });
      }
      section.rows.push_back(std::move(opening));
      auto const add = [&section](NoteRow& row) {
        if (row.amount != 0) {
          section.rows.push_back(std::move(row));
        }
      };
      if (disposal_first(section.text)) {
        add(disposal);
        add(other);
      } else {
        add(other);
        add(disposal);
      }
      section.rows.push_back(std::move(closing));
      Amount sum = 0;
      for (std::size_t row = 0; row + 1 < section.rows.size(); ++row) {
        sum += section.rows[row].amount;
      }
      if (sum != section.rows.back().amount) {
        notes.errors.push_back(std::format("{}: {} opens and changes to {}, but closes at {}", note.title, section.text,
                                           format_amount(sum), format_amount(section.rows.back().amount)));
      }
      note.sum += section.rows.back().amount;
    }
    if (auto const post = post_of(note.title)) {
      auto const found = std::ranges::find_if(lines, [&post](StatementLine const& line) {
        return !line.in_notes && line.kind == LineKind::item && line.text == *post;
      });
      if (found == lines.end()) {
        notes.errors.push_back(std::format("{}: no post {} in the balansräkning", note.title, *post));
      } else if (found->exact.front() != note.sum) {
        notes.errors.push_back(std::format("{}: {} {} differs from {} {} in the balansräkning", note.title,
                                           note.sum_text, format_amount(note.sum), *post,
                                           format_amount(found->exact.front())));
      }
    }
    notes.notes.push_back(std::move(note));
  }
  return notes;
}

std::map<std::string, std::string> note_references(Notes const& notes) {
  std::map<std::string, std::string> references;
  for (auto const& note : notes.notes) {
    if (auto const post = post_of(note.title)) {
      std::string_view const title = note.title;
      references.emplace(*post, title.substr(4, title.size() - post->size() - 5));
    }
  }
  return references;
}

} // namespace cratchit::ar
