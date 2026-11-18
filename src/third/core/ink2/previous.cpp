#include "core/ink2/previous.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <chrono>
#include <format>
#include <set>

namespace cratchit::ink2 {

namespace {

std::optional<Kronor> kronor_of(std::string_view text) {
  Kronor value = 0;
  auto const [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
  if (error != std::errc{} || end != text.data() + text.size()) {
    return std::nullopt;
  }
  return value;
}

std::string lower(std::string text) {
  std::ranges::transform(text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return text;
}

} // namespace

std::expected<std::filesystem::path, std::string> sru_file_in(std::filesystem::path const& path) {
  std::error_code error;
  if (std::filesystem::is_regular_file(path, error)) {
    return path;
  }
  if (!std::filesystem::is_directory(path, error)) {
    return std::unexpected(std::format("No file or folder {}", path.string()));
  }
  for (auto const& entry : std::filesystem::directory_iterator(path, error)) {
    if (entry.is_regular_file() && lower(entry.path().filename().string()) == "blanketter.sru") {
      return entry.path();
    }
  }
  return std::unexpected(std::format("No BLANKETTER.SRU in {}", path.string()));
}

std::string orgnr_digits(std::string_view orgnr) {
  std::string digits;
  std::ranges::copy_if(orgnr, std::back_inserter(digits), [](char c) { return c >= '0' && c <= '9'; });
  if (digits.size() == 12 && digits.starts_with("16")) {
    digits.erase(0, 2);
  }
  return digits;
}

std::expected<Previous, std::string> previous_from_sru(std::vector<SruForm> const& forms, std::string_view source,
                                                       std::string_view orgnr, Date start) {
  auto const error = [source](std::string_view what) {
    return std::unexpected(std::format("Last year's return {}: {}", source, what));
  };
  auto const ink2s = std::ranges::find(forms, "INK2S", &SruForm::kind);
  if (ink2s == forms.end()) {
    return error("no INK2S");
  }
  auto const identity = ink2s->identity.substr(0, ink2s->identity.find(' '));
  if (orgnr_digits(identity) != orgnr_digits(orgnr)) {
    return error(std::format("orgnr {}, not {}", identity, orgnr));
  }
  auto const day_before = format_compact_date(Date{std::chrono::sys_days{start} - std::chrono::days{1}});
  auto const end = ink2s->fields.find("7012");
  if (end == ink2s->fields.end() || end->second != day_before) {
    return error(std::format("its fiscal year ends {}, not {} (the day before this one starts)",
                             end == ink2s->fields.end() ? "(not given)" : end->second, day_before));
  }
  Previous previous;
  previous.source = std::string(source);
  if (auto const deficit = ink2s->fields.find("7770"); deficit != ink2s->fields.end()) {
    auto const amount = kronor_of(deficit->second);
    if (!amount || *amount < 0) {
      return error(std::format("4.16 (7770) \"{}\" is not an amount", deficit->second));
    }
    previous.deficit = *amount;
  }
  for (std::string_view const mark : {"8040", "8041", "8044", "8045"}) {
    if (auto const given = ink2s->fields.find(std::string(mark)); given != ink2s->fields.end() && given->second == "X") {
      previous.marks.emplace_back(mark);
    }
  }
  if (auto const ink2r = std::ranges::find(forms, "INK2R", &SruForm::kind); ink2r != forms.end()) {
    previous.ink2r = *ink2r;
  }
  return previous;
}

Previous previous_deficit(Kronor deficit, std::string_view source) {
  return {deficit, std::string(source), {}, std::nullopt};
}

Balances opening_balances(sie::Document const& document) {
  Balances balances;
  for (auto const& balance : document.opening) {
    if (balance.year == 0) {
      balances[balance.account] += balance.amount;
    }
  }
  return balances;
}

std::vector<std::string> check_opening(Form const& opening, SruForm const& last_ink2r) {
  std::set<std::string> codes;
  for (auto const& [code, value] : last_ink2r.fields) {
    if (code.starts_with("72") || code.starts_with("73")) {
      codes.insert(code);
    }
  }
  for (auto const& [code, field] : opening.fields) {
    if (field.used && (code.starts_with("72") || code.starts_with("73"))) {
      codes.insert(code);
    }
  }
  std::vector<std::string> notes;
  for (auto const& code : codes) {
    auto const filed = last_ink2r.fields.find(code);
    auto const last = filed == last_ink2r.fields.end() ? Kronor{0} : kronor_of(filed->second).value_or(0);
    auto const field = opening.fields.find(code);
    auto const now = field == opening.fields.end() || !field->second.used ? Kronor{0} : field->second.kronor;
    if (last != now) {
      notes.push_back(std::format("Opening balance: {} is {} kr by this year's #IB but {} kr in last year's INK2R",
                                  code, now, last));
    }
  }
  return notes;
}

} // namespace cratchit::ink2
