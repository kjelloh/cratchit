#include "ui/year_view.hpp"

#include "core/amount.hpp"
#include "core/date.hpp"

#include <algorithm>
#include <format>
#include <ranges>

namespace cratchit::ui {

namespace {

struct Row {
  Date date;
  std::string text;
};

std::string row_text(std::string_view first, Date date, Amount amount, std::string_view heading) {
  return std::format("  {:<13} {}  {:>12}  {}", first, format_iso_date(date), format_amount(amount), heading);
}

// The amount a draft shows: its event's, with its sign (minus for an event the other way,
// e.g. a credit note); without one, its only posting's with its sign (in or out of the
// account, as a statement entry is); else what its postings move (without sign)
Amount shown_amount(books::Draft const& draft) {
  if (draft.event.amount != 0) return draft.event.amount;
  books::Posting const* only = nullptr;
  std::size_t counted = 0;
  for (auto const& posting : draft.postings) {
    if (posting.kind == books::Posting::Kind::removed) continue;
    only = &posting;
    ++counted;
  }
  if (counted == 1) return only->amount;
  return std::max(books::debit(draft.postings), books::credit(draft.postings));
}

std::string summary_of(books::FiscalYear const* year) {
  auto const vouchers = year != nullptr ? year->vouchers.size() : 0;
  auto const drafts = year != nullptr ? year->drafts.size() : 0;
  auto text = std::string("Hela året: ");
  if (vouchers == 0) {
    text += "inget bokfört";
  } else {
    text += std::format("{} {}", vouchers, vouchers == 1 ? "verifikation" : "verifikationer");
  }
  if (drafts > 0) text += std::format(", {} utkast", drafts);
  return text;
}

// Code points of UTF-8 text (as wide as they show, for the texts cratchit meets)
std::size_t code_points(std::string_view text) {
  return static_cast<std::size_t>(
      std::ranges::count_if(text, [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; }));
}

// text cut to at most max code points: the first max - 2, spaces at the end dropped, then ".."
std::string cut_to(std::string_view text, std::size_t max) {
  if (code_points(text) <= max) return std::string(text);
  std::string cut;
  std::size_t count = 0;
  for (char const c : text) {
    bool const starts = (static_cast<unsigned char>(c) & 0xC0) != 0x80;
    if (starts && ++count > (max > 2 ? max - 2 : 0)) break;
    cut += c;
  }
  while (!cut.empty() && cut.back() == ' ') cut.pop_back();
  return cut + "..";
}

// The parts joined by " : " in at most width code points (beslut 2026-11-18): a part
// shorter than its share stays whole, what is left is shared equally by the longer ones
std::string joined_within(std::vector<std::string> const& parts, std::size_t width) {
  constexpr std::string_view separator = " : ";
  auto const separators = parts.empty() ? 0 : separator.size() * (parts.size() - 1);
  auto room = width > separators ? width - separators : 0;
  std::vector<std::size_t> share(parts.size(), 0);
  std::vector<std::size_t> left(parts.size());
  for (std::size_t i = 0; i < parts.size(); ++i) left[i] = i;
  while (!left.empty()) {
    auto const each = room / left.size();
    std::vector<std::size_t> longer;
    for (auto const i : left) {
      if (code_points(parts[i]) <= each) {
        share[i] = code_points(parts[i]);
        room -= share[i];
      } else {
        longer.push_back(i);
      }
    }
    if (longer.size() == left.size()) {
      for (auto const i : longer) share[i] = each;
      break;
    }
    left = std::move(longer);
  }
  std::string text;
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i > 0) text += separator;
    text += cut_to(parts[i], share[i]);
  }
  return text;
}

} // namespace

std::string_view stage_text(books::DraftStage stage) {
  switch (stage) {
  case books::DraftStage::event:
    return "händelse";
  case books::DraftStage::account_event:
    return "kontohändelse";
  case books::DraftStage::same:
    return "samma";
  case books::DraftStage::unbalanced:
    return "obalanserat";
  case books::DraftStage::ready:
    return "klart";
  }
  return "";
}

std::string same_text(books::Draft const& draft) {
  if (books::stage(draft) != books::DraftStage::same) return {};
  std::vector<std::string> parts;
  if (!draft.event.heading.empty()) parts.push_back(draft.event.heading);
  for (auto const& posting : draft.postings) {
    if (posting.kind == books::Posting::Kind::removed || posting.text.empty()) continue;
    parts.push_back(posting.text);
  }
  return joined_within(parts, same_text_width);
}

std::string draft_text(books::Draft const& draft) {
  auto const stage = books::stage(draft);
  // The same event from several parts: their raw texts, in the best case the text of the
  // voucher to come (beslut 2026-11-18); worked out each time, never stored
  std::string text = stage == books::DraftStage::same ? same_text(draft) : draft.event.heading;
  if (text.empty()) {
    auto const posting = std::ranges::find_if(draft.postings, [](books::Posting const& p) {
      return p.kind != books::Posting::Kind::removed;
    });
    if (posting != draft.postings.end()) {
      text = posting->text.empty() ? posting->account : std::format("{}  {}", posting->account, posting->text);
    }
  }
  auto const shown_stage = std::format("({})", stage_text(stage));
  return text.empty() ? shown_stage : std::format("{} {}", text, shown_stage);
}

std::string draft_id(std::size_t index, std::size_t count) {
  return std::format("#{}", index < count ? count - index : 0);
}

std::optional<std::size_t> draft_index(std::size_t number, std::size_t count) {
  if (number == 0 || number > count) return std::nullopt;
  return count - number;
}

std::size_t draft_count(std::filesystem::path const& folder, Date start) {
  auto const books = books::read_books(folder);
  if (!books) return 0;
  auto const* year = books->year_starting(start);
  return year != nullptr ? year->drafts.size() : 0;
}

std::string draft_row(std::size_t index, std::size_t count, books::Draft const& draft) {
  return row_text(draft_id(index, count), draft.event.date, shown_amount(draft), draft_text(draft));
}

std::string voucher_row(books::Voucher const& voucher) {
  return row_text(books::voucher_id(voucher.series, voucher.number), voucher.event.date, voucher.event.amount,
                  voucher.event.heading);
}

std::vector<std::string> draft_rows(books::FiscalYear const* year) {
  std::vector<std::string> lines;
  if (year == nullptr) return lines;
  for (std::size_t i = year->drafts.size(); i-- > 0;) lines.push_back(draft_row(i, year->drafts.size(), year->drafts[i]));
  return lines;
}

std::vector<std::string> voucher_rows(books::FiscalYear const* year) {
  std::vector<std::string> lines;
  if (year == nullptr) return lines;
  std::vector<Row> rows;
  for (auto const& voucher : year->vouchers) {
    rows.push_back({voucher.event.date, voucher_row(voucher)});
  }
  std::ranges::stable_sort(rows, {}, &Row::date);
  for (auto const& row : rows | std::views::reverse) lines.push_back(row.text);
  return lines;
}

YearView year_view(books::FiscalYear const* year, std::size_t max_rows) {
  YearView view{summary_of(year), {}};
  if (year == nullptr) return view;

  // Oldest first: by date, vouchers in the order of the books, then the drafts of the same date
  std::vector<Row> rows;
  for (auto const& voucher : year->vouchers) {
    rows.push_back({voucher.event.date, row_text(books::voucher_id(voucher.series, voucher.number),
                                                 voucher.event.date, voucher.event.amount, voucher.event.heading)});
  }
  for (auto const& draft : year->drafts) {
    rows.push_back({draft.event.date, row_text(stage_text(stage(draft)), draft.event.date, shown_amount(draft),
                                               draft.event.heading)});
  }
  std::ranges::stable_sort(rows, {}, &Row::date);

  auto const shown = std::min(rows.size(), max_rows);
  for (auto const& row : rows | std::views::reverse | std::views::take(shown)) {
    view.rows.push_back(row.text);
  }
  if (rows.size() > shown) view.rows.push_back(std::format("  … {} äldre", rows.size() - shown));
  return view;
}

} // namespace cratchit::ui
