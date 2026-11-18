#include "core/same_event.hpp"

#include <algorithm>
#include <chrono>
#include <format>
#include <map>
#include <ranges>
#include <set>
#include <tuple>
#include <utility>

namespace cratchit {

namespace {

int days_between(Date a, Date b) {
  auto const days = (std::chrono::sys_days{b} - std::chrono::sys_days{a}).count();
  return static_cast<int>(days < 0 ? -days : days);
}

bool counted(books::Posting const& posting) { return posting.kind != books::Posting::Kind::removed; }

bool has_postings(books::Draft const& draft) { return std::ranges::any_of(draft.postings, counted); }

bool is_digit(char c) { return c >= '0' && c <= '9'; }

// ASCII A-Z and the Latin-1 letters in UTF-8 (À-Þ, not ×) to lower case
std::string folded(std::string_view text) {
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

std::set<std::string> words_of(std::string_view text) {
  std::set<std::string> words;
  std::string word;
  auto const add = [&] {
    if (word.size() > 1 && !std::ranges::all_of(word, is_digit)) words.insert(folded(word));
    word.clear();
  };
  for (char const c : text) {
    auto const u = static_cast<unsigned char>(c);
    if (u >= 0x80 || is_digit(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
      word += c;
    } else {
      add();
    }
  }
  add();
  return words;
}

bool has_event(books::Draft const& draft) { return !draft.event.heading.empty() || draft.event.amount != 0; }

// Two drafts that can be the same event (beslut 2026-11-17): at most one has an
// event; when both have postings, they are on accounts the other has none on
// and balance together (money out of one account and into another); else one
// is just the event and the other has the postings (signs are not compared: an
// event's sign tells its direction, a posting's in or out of the account)
bool fit_together(books::Draft const& a, books::Draft const& b) {
  if (has_event(a) && has_event(b)) return false;
  auto const a_postings = has_postings(a);
  auto const b_postings = has_postings(b);
  if (a_postings != b_postings) return true;
  if (!a_postings) return false;
  for (auto const& posting : a.postings) {
    if (!counted(posting)) continue;
    if (std::ranges::any_of(b.postings, [&](books::Posting const& other) {
          return counted(other) && other.account == posting.account;
        })) {
      return false;
    }
  }
  auto const postings = merged(a, b, a.event).postings;
  return books::sum(postings) == 0;
}

} // namespace

Amount amount_of(books::Draft const& draft) {
  if (draft.event.amount != 0) return draft.event.amount < 0 ? -draft.event.amount : draft.event.amount;
  return std::max(books::debit(draft.postings), books::credit(draft.postings));
}

std::string compared_text(books::Draft const& draft) {
  if (!draft.event.heading.empty()) return draft.event.heading;
  std::string text;
  for (auto const& posting : draft.postings) {
    if (!counted(posting) || posting.text.empty()) continue;
    if (!text.empty()) text += ' ';
    text += posting.text;
  }
  return text;
}

std::vector<std::string> shared_words(std::string_view a, std::string_view b) {
  auto const in_b = words_of(b);
  std::vector<std::string> shared;
  for (auto const& word : words_of(a)) {
    if (in_b.contains(word)) shared.push_back(word);
  }
  return shared;
}

std::vector<SameEvent> find_same_events(books::FiscalYear const& year, int window_days) {
  std::vector<SameEvent> pairs;
  for (std::size_t i = 0; i < year.drafts.size(); ++i) {
    auto const& draft = year.drafts[i];
    auto const amount = amount_of(draft);
    if (amount == 0) continue;
    for (std::size_t j = i + 1; j < year.drafts.size(); ++j) {
      auto const& other = year.drafts[j];
      auto const days = days_between(draft.event.date, other.event.date);
      if (amount_of(other) == amount && days <= window_days && fit_together(draft, other)) {
        pairs.push_back({SameEvent::Kind::drafts, i, j, days, shared_words(compared_text(draft), compared_text(other)),
                         true});
      }
    }
    for (std::size_t v = 0; v < year.vouchers.size(); ++v) {
      auto const& voucher = year.vouchers[v];
      auto const days = days_between(draft.event.date, voucher.event.date);
      if (voucher.event.amount == amount && days <= window_days) {
        pairs.push_back({SameEvent::Kind::booked, i, v, days,
                         shared_words(compared_text(draft), voucher.event.heading),
                         has_postings(draft) && unbooked(draft, voucher).empty()});
      }
    }
  }
  std::ranges::stable_sort(pairs, [](SameEvent const& a, SameEvent const& b) {
    if (a.fits != b.fits) return a.fits;
    if (a.words.size() != b.words.size()) return a.words.size() > b.words.size();
    return std::tie(a.days, a.draft) < std::tie(b.days, b.draft);
  });
  return pairs;
}

books::Postings unbooked(books::Draft const& draft, books::Voucher const& voucher) {
  std::map<std::tuple<std::string, Date, Amount>, std::size_t> booked;
  for (auto const& posting : voucher.postings) {
    if (counted(posting)) ++booked[{posting.account, posting.date.value_or(voucher.event.date), posting.amount}];
  }
  books::Postings left;
  for (auto const& posting : draft.postings) {
    if (!counted(posting)) continue;
    auto const it = booked.find({posting.account, posting.date.value_or(draft.event.date), posting.amount});
    if (it != booked.end() && it->second > 0) {
      --it->second;
    } else {
      left.push_back(posting);
    }
  }
  return left;
}

books::Draft merged(books::Draft const& a, books::Draft const& b, books::Event event) {
  books::Draft result{std::move(event), std::min(a.noted, b.noted), a.postings};
  for (auto const& posting : b.postings) {
    bool const same_entry = !posting.source.empty() && std::ranges::any_of(a.postings, [&](books::Posting const& kept) {
      return kept.source == posting.source;
    });
    if (!same_entry) result.postings.push_back(posting);
  }
  return result;
}

DraftRemoval removal_of(books::Draft const& draft) {
  auto const entries = std::ranges::any_of(draft.postings, [](books::Posting const& posting) {
    return counted(posting) && !posting.source.empty();
  });
  if (!entries) return DraftRemoval::forget;
  switch (parts_of(draft)) {
  case DraftParts::lone: return DraftRemoval::forget;
  case DraftParts::same: return DraftRemoval::split;
  case DraftParts::more: return DraftRemoval::none;
  }
  return DraftRemoval::none;
}

std::vector<books::Draft> split(books::Draft const& draft) {
  std::vector<books::Draft> drafts;
  if (!draft.event.heading.empty() || draft.event.amount != 0) drafts.push_back({draft.event, draft.noted, {}});
  for (auto const& posting : draft.postings) {
    if (!counted(posting)) continue;
    drafts.push_back({{"", 0, posting.date.value_or(draft.event.date)}, draft.noted, {posting}});
  }
  return drafts;
}

std::expected<std::size_t, std::string> split_draft(std::filesystem::path const& project_folder, Date year_start,
                                                    std::size_t index, books::Draft const& expected) {
  auto books = books::read_books(project_folder);
  if (!books) return std::unexpected(books.error());
  auto const year = std::ranges::find_if(books->years, [&](books::FiscalYear const& y) { return y.start == year_start; });
  if (year == books->years.end()) {
    return std::unexpected(std::format("Räkenskapsåret som börjar {} finns inte i bokföringen", format_iso_date(year_start)));
  }
  if (index >= year->drafts.size() || year->drafts[index] != expected) {
    return std::unexpected("Utkastet finns inte längre på sin plats i bokföringen. Öppna det igen från Utkast.");
  }
  if (parts_of(expected) != DraftParts::same) {
    return std::unexpected("Utkastet är inte gjort av delar med samma belopp och kan inte delas.");
  }
  auto parts = split(expected);
  auto const count = parts.size();
  year->drafts.erase(year->drafts.begin() + static_cast<std::ptrdiff_t>(index));
  for (auto& part : parts) year->place_draft(std::move(part));
  if (auto written = books::write_books(project_folder, *books); !written) return std::unexpected(written.error());
  return count;
}

std::expected<std::size_t, std::string> merge_drafts(std::filesystem::path const& project_folder, Date year_start,
                                                     std::size_t a, books::Draft const& expected_a, std::size_t b,
                                                     books::Draft const& expected_b, books::Event const& event) {
  auto books = books::read_books(project_folder);
  if (!books) return std::unexpected(books.error());
  auto const year = std::ranges::find_if(books->years, [&](books::FiscalYear const& y) { return y.start == year_start; });
  if (year == books->years.end()) {
    return std::unexpected(std::format("Räkenskapsåret som börjar {} finns inte i bokföringen", format_iso_date(year_start)));
  }
  if (a == b || a >= year->drafts.size() || b >= year->drafts.size() || year->drafts[a] != expected_a ||
      year->drafts[b] != expected_b) {
    return std::unexpected("Utkasten finns inte längre på sina platser i bokföringen. Välj paret igen från Samma?.");
  }
  auto one = merged(expected_a, expected_b, event);
  year->drafts.erase(year->drafts.begin() + static_cast<std::ptrdiff_t>(std::max(a, b)));
  year->drafts.erase(year->drafts.begin() + static_cast<std::ptrdiff_t>(std::min(a, b)));
  auto const index = year->place_draft(std::move(one));
  if (auto written = books::write_books(project_folder, *books); !written) return std::unexpected(written.error());
  return index;
}

} // namespace cratchit
