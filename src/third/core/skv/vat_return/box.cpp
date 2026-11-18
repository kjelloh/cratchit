#include "core/skv/vat_return/box.hpp"

#include "core/log.hpp"
#include "core/skv/vat_return/account_boxes.hpp"

#include <algorithm>
#include <cctype>
#include <format>
#include <map>
#include <utility>
#include <vector>

namespace cratchit::skv::vat_return {

namespace {

bool is_digits(std::string_view text, std::size_t count) {
  return text.size() == count &&
         std::ranges::all_of(text, [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; });
}

std::vector<std::string_view> cells_of(std::string_view line) {
  std::vector<std::string_view> cells;
  for (std::size_t start = 0;;) {
    auto const end = line.find(';', start);
    cells.push_back(line.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start));
    if (end == std::string_view::npos) {
      return cells;
    }
    start = end + 1;
  }
}

} // namespace

std::expected<std::map<std::string, Box>, std::string> account_boxes(std::string_view text) {
  if (text.starts_with("\xEF\xBB\xBF")) {
    text.remove_prefix(3);
  }
  std::map<std::string, Box> boxes;
  std::size_t number = 0;
  while (!text.empty()) {
    ++number;
    auto const end = text.find('\n');
    auto line = text.substr(0, end);
    text.remove_prefix(end == std::string_view::npos ? text.size() : end + 1);
    if (line.ends_with('\r')) {
      line.remove_suffix(1);
    }
    if (line.empty()) {
      continue;
    }
    auto const cells = cells_of(line);
    if (number == 1 && cells[0] == "in") {
      continue;
    }
    if (cells.size() >= 3 && cells[0].empty() && !cells[2].empty()) {
      continue; // an account in `out` is a source of the box, not added up into it
    }
    if (cells.size() < 2 || !is_digits(cells[0], 4)) {
      return std::unexpected(std::format("Line {}: the account is not four digits: {}", number, line));
    }
    if (!is_digits(cells[1], 2)) {
      return std::unexpected(std::format("Line {}: the box is not two digits: {}", number, line));
    }
    if (!boxes.emplace(std::string(cells[0]), Box(cells[1])).second) {
      return std::unexpected(std::format("Line {}: account {} is listed before", number, cells[0]));
    }
  }
  return boxes;
}

namespace {

std::map<std::string, Box> const& built_in_boxes() {
  static auto const boxes = [] {
    auto parsed = account_boxes(account_boxes_text());
    if (!parsed) {
      LOG_DESIGN_INSUFFICIENCY("Built-in VAT return boxes of accounts not read: {}", parsed.error());
      return std::map<std::string, Box>{};
    }
    return *std::move(parsed);
  }();
  return boxes;
}

} // namespace

std::vector<std::string> accounts_of(Box const& box) {
  std::vector<std::string> accounts;
  for (auto const& [account, of] : built_in_boxes()) {
    if (of == box) {
      accounts.push_back(account);
    }
  }
  return accounts;
}

std::optional<Box> box_of(std::string_view account) {
  auto const& boxes = built_in_boxes();
  if (auto const found = boxes.find(std::string(account)); found != boxes.end()) {
    return found->second;
  }
  return std::nullopt;
}

bool is_vat_account(std::string_view account) {
  auto const box = box_of(account);
  return box && std::ranges::find(vat_boxes, *box) != vat_boxes.end();
}

bool is_vat_clearing_account(std::string_view account) {
  return account == vat_liability_account || account == vat_receivable_account;
}

} // namespace cratchit::skv::vat_return
