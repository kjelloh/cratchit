#include "core/skv/vat_return/box_texts.hpp"

#include "core/csv.hpp"
#include "core/log.hpp"

#include <algorithm>
#include <format>

namespace cratchit::skv::vat_return {

std::expected<std::vector<BoxText>, std::string> box_texts(std::string_view text) {
  if (text.starts_with("\xEF\xBB\xBF")) {
    text.remove_prefix(3);
  }
  auto const rows = parse_csv(text, ';');
  if (!rows) {
    return std::unexpected(rows.error());
  }
  std::vector<BoxText> texts;
  for (std::size_t i = 0; i < rows->size(); ++i) {
    auto const& row = (*rows)[i];
    if ((i == 0 && !row.empty() && row[0] == "Box") || row.empty() || row[0].empty()) {
      continue;
    }
    auto const& box = row[0];
    if (box.size() != 2 || !std::ranges::all_of(box, [](char c) { return c >= '0' && c <= '9'; })) {
      return std::unexpected(std::format("Row {}: the box is not two digits: {}", i + 1, box));
    }
    if (row.size() < 3 || row[1].empty() || row[2].empty()) {
      return std::unexpected(std::format("Row {}: box {} has no section or title", i + 1, box));
    }
    std::string description = row.size() > 3 ? row[3] : std::string{};
    std::erase(description, '\r');
    texts.push_back({box, row[1], row[2], std::move(description)});
  }
  return texts;
}

std::vector<BoxText> const& built_in_box_texts() {
  static auto const texts = [] {
    auto parsed = box_texts(box_texts_text());
    if (!parsed) {
      LOG_DESIGN_INSUFFICIENCY("Built-in VAT return box texts not read: {}", parsed.error());
      return std::vector<BoxText>{};
    }
    return *std::move(parsed);
  }();
  return texts;
}

std::size_t form_position(Box const& box) {
  auto const& texts = built_in_box_texts();
  auto const it = std::ranges::find(texts, box, &BoxText::box);
  return it == texts.end() ? texts.size() : static_cast<std::size_t>(it - texts.begin());
}

std::optional<BoxText> text_of(Box const& box, Amount amount) {
  auto const& texts = built_in_box_texts();
  std::vector<BoxText const*> found;
  for (auto const& text : texts) {
    if (text.box == box) {
      found.push_back(&text);
    }
  }
  if (found.empty()) {
    return std::nullopt;
  }
  return *found[box == "48" && amount < 0 && found.size() > 1 ? 1 : 0];
}

} // namespace cratchit::skv::vat_return
