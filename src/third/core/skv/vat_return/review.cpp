#include "core/skv/vat_return/review.hpp"

#include "core/skv/vat_return/box_texts.hpp"

#include <algorithm>
#include <format>
#include <set>

namespace cratchit::skv::vat_return {

namespace {

std::size_t characters(std::string_view text) {
  return static_cast<std::size_t>(
      std::ranges::count_if(text, [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; }));
}

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

std::string account_name(books::Books const& books, std::string const& account) {
  auto const* known = books.account(account);
  return known ? known->name : std::string{};
}

} // namespace

std::vector<ReviewBox> review_boxes(Document const& document) {
  std::vector<ReviewBox> boxes;
  for (auto const& text : built_in_box_texts()) {
    if (text.box == vat_to_pay_box || std::ranges::find(boxes, text.box, &ReviewBox::box) != boxes.end()) {
      continue;
    }
    std::size_t postings = 0;
    if (auto const posted = std::ranges::find(document.boxes, text.box, &BoxAmount::box);
        posted != document.boxes.end()) {
      for (auto const& account : posted->accounts) {
        postings += account.postings.size();
      }
    }
    boxes.push_back({text.box, text.section, text.title, postings});
  }
  return boxes;
}

std::vector<std::string> review_of(books::Books const& books, Document const& document, Box const& box,
                                   std::size_t width) {
  auto const posted = std::ranges::find(document.boxes, box, &BoxAmount::box);
  std::vector<BoxText const*> texts; // the box's titles, the main one first
  for (auto const& text : built_in_box_texts()) {
    if (text.box == box) {
      texts.push_back(&text);
    }
  }
  std::vector<std::string> lines;
  lines.push_back(std::format("Ruta {}  {}", box, texts.empty() ? std::string{} : texts.front()->title));
  for (std::size_t i = 1; i < texts.size(); ++i) {
    lines.push_back(std::format("Även: {}", texts[i]->title));
  }
  if (!texts.empty()) {
    lines.push_back(std::format("Avsnitt: {}", texts.front()->section));
  }
  std::set<std::string> with_postings;
  if (posted == document.boxes.end()) {
    lines.push_back(std::format("Inga konteringar {} – {}.", format_iso_date(document.period.first),
                                format_iso_date(document.period.last)));
  } else {
    lines.push_back(std::format("På blanketten: {}", format_amount(posted->amount)));
    lines.push_back("Konton och verifikationer som i bokföringen (debet positivt, kredit negativt):");
    for (auto const& account : posted->accounts) {
      with_postings.insert(account.account);
      lines.push_back(std::format("  {:<6}{:>15}  {}", account.account, format_amount(account.amount),
                                  account_name(books, account.account)));
      for (auto const& posting : account.postings) {
        lines.push_back(std::format("        {:<8}{}{:>15}  {}", books::voucher_id(posting.series, posting.number),
                                    format_iso_date(posting.date), format_amount(posting.amount),
                                    heading_of(books, posting)));
      }
    }
  }
  std::vector<std::string> others;
  for (auto const& account : accounts_of(box)) {
    if (!with_postings.contains(account)) {
      auto const name = account_name(books, account);
      others.push_back(std::format("  {:<6}{}", account, name.empty() ? "(finns inte i kontoplanen)" : name));
    }
  }
  if (!others.empty()) {
    lines.push_back(with_postings.empty() ? std::format("Konton som redovisas i ruta {}:", box)
                                          : std::format("Andra konton som redovisas i ruta {}, utan konteringar:", box));
    lines.insert(lines.end(), others.begin(), others.end());
  }
  lines.push_back("");
  lines.push_back(std::format("Skatteverkets beskrivning av ruta {}:", box));
  for (std::size_t i = 0; i < texts.size(); ++i) {
    if (i > 0) {
      lines.push_back("");
    }
    for (auto& line : wrap_text(texts[i]->description, width)) {
      lines.push_back(std::move(line));
    }
  }
  lines.push_back("");
  return lines;
}

std::string cut_text(std::string_view text, std::size_t max) {
  if (characters(text) <= max) {
    return std::string(text);
  }
  std::size_t count = 0;
  std::size_t end = 0;
  for (; end < text.size(); ++end) {
    if ((static_cast<unsigned char>(text[end]) & 0xC0) != 0x80 && count++ == max - 3) {
      break;
    }
  }
  return std::string(text.substr(0, end)) + "...";
}

std::vector<std::string> wrap_text(std::string_view text, std::size_t width) {
  std::vector<std::string> lines;
  for (std::size_t start = 0; start <= text.size();) {
    auto const end = std::min(text.find('\n', start), text.size());
    auto const paragraph = text.substr(start, end - start);
    std::string line;
    std::size_t length = 0;
    bool words = false;
    for (std::size_t i = 0; i < paragraph.size();) {
      auto const space = std::min(paragraph.find(' ', i), paragraph.size());
      auto const word = paragraph.substr(i, space - i);
      i = space + 1;
      if (word.empty()) {
        continue;
      }
      words = true;
      auto const size = characters(word);
      if (length > 0 && length + 1 + size > width) {
        lines.push_back(std::move(line));
        line.clear();
        length = 0;
      }
      if (length > 0) {
        line += ' ';
        ++length;
      }
      line += word;
      length += size;
    }
    if (words) {
      lines.push_back(std::move(line));
    } else if (end < text.size()) {
      lines.emplace_back(); // an empty line stays
    }
    start = end + 1;
  }
  return lines;
}

} // namespace cratchit::skv::vat_return
