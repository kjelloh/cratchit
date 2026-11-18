#include "core/ink2/accounts.hpp"

#include <algorithm>
#include <charconv>
#include <format>
#include <optional>

namespace cratchit::ink2 {

namespace {

constexpr std::string_view en_dash = "–";

std::optional<int> account_number(std::string_view text) {
  if (text.size() != 4 || !std::ranges::all_of(text, [](char c) { return c >= '0' && c <= '9'; })) {
    return std::nullopt;
  }
  int number = 0;
  std::from_chars(text.data(), text.data() + text.size(), number);
  return number;
}

bool in_any(std::vector<std::pair<int, int>> const& ranges, int number) {
  return std::ranges::any_of(ranges, [number](auto const& range) { return range.first <= number && number <= range.second; });
}

// Reads the account set text left to right
class SetReader {
public:
  explicit SetReader(std::string_view text) : text_(text) {}

  std::expected<AccountSet, std::string> read() {
    AccountSet set;
    std::optional<Condition> leading;
    if (take("+")) {
      leading = Condition::profit;
    } else if (take(en_dash) || take("-")) {
      leading = Condition::loss;
    }
    AccountGroup current;
    std::size_t unconditioned = 0; // the first group the next condition applies to
    auto const close = [&] {
      if (!current.included.empty()) {
        set.groups.push_back(std::move(current));
      }
      current = {};
    };
    while (true) {
      if (!read_term(current.included)) {
        return error();
      }
      // After a term: a separator and the next term, or any of "(exkl. ...)" and "(Om netto ...)"
      while (true) {
        if (take("(")) {
          if (take_word("exkl.")) {
            if (current.included.empty() || !read_list(current.excluded) || !take(")")) {
              return error();
            }
            close();
          } else if (take_word("Om")) {
            auto const condition = take_word("netto") ? read_sign() : std::nullopt;
            if (!condition || !take(")")) {
              return error();
            }
            close();
            if (unconditioned == set.groups.size()) {
              return error();
            }
            for (auto index = unconditioned; index < set.groups.size(); ++index) {
              set.groups[index].condition = *condition;
            }
            unconditioned = set.groups.size();
          } else {
            return error();
          }
        } else {
          break;
        }
      }
      if (skip_space(), at_end()) {
        break;
      }
      if (!take(",")) {
        return error();
      }
    }
    close();
    if (leading) {
      if (unconditioned != 0) {
        return error();
      }
      for (auto& group : set.groups) {
        group.condition = *leading;
      }
    }
    return set;
  }

private:
  std::unexpected<std::string> error() const {
    return std::unexpected(std::format("Unreadable BAS accounts \"{}\" at \"{}\"", text_, text_.substr(position_)));
  }

  bool at_end() const { return position_ >= text_.size(); }

  void skip_space() {
    while (!at_end() && text_[position_] == ' ') {
      ++position_;
    }
  }

  bool take(std::string_view what) {
    skip_space();
    if (!text_.substr(position_).starts_with(what)) {
      return false;
    }
    position_ += what.size();
    return true;
  }

  bool take_word(std::string_view word) {
    auto const start = position_;
    if (take(word) && (at_end() || text_[position_] == ' ' || text_[position_] == ')')) {
      return true;
    }
    position_ = start;
    return false;
  }

  std::optional<Condition> read_sign() {
    if (take("+")) {
      return Condition::credit;
    }
    if (take(en_dash) || take("-")) {
      return Condition::debit;
    }
    return std::nullopt;
  }

  // Four characters: digits, then any number of x. The first and last account it stands for.
  std::optional<std::pair<int, int>> take_pattern() {
    skip_space();
    if (text_.size() - position_ < 4) {
      return std::nullopt;
    }
    auto const pattern = text_.substr(position_, 4);
    auto const digits = pattern.find('x');
    auto const prefix = pattern.substr(0, digits);
    if (prefix.empty() || !std::ranges::all_of(prefix, [](char c) { return c >= '0' && c <= '9'; }) ||
        (digits != std::string_view::npos &&
         !std::ranges::all_of(pattern.substr(digits), [](char c) { return c == 'x'; }))) {
      return std::nullopt;
    }
    // A fifth digit or x means it is not an account
    if (position_ + 4 < text_.size()) {
      auto const next = text_[position_ + 4];
      if ((next >= '0' && next <= '9') || next == 'x') {
        return std::nullopt;
      }
    }
    position_ += 4;
    std::string first(pattern);
    std::string last(pattern);
    std::ranges::replace(first, 'x', '0');
    std::ranges::replace(last, 'x', '9');
    return std::pair{*account_number(first), *account_number(last)};
  }

  // A pattern, or a range of two
  bool read_term(std::vector<std::pair<int, int>>& ranges) {
    auto const first = take_pattern();
    if (!first) {
      return false;
    }
    auto range = *first;
    auto const before = position_;
    if (take(en_dash) || take("-")) {
      auto const last = take_pattern();
      if (!last || last->second < first->first) {
        position_ = before;
        return false;
      }
      range.second = last->second;
    }
    ranges.push_back(range);
    return true;
  }

  // Terms separated by "," or "och"
  bool read_list(std::vector<std::pair<int, int>>& ranges) {
    while (true) {
      if (!read_term(ranges)) {
        return false;
      }
      if (!take(",") && !take_word("och")) {
        return true;
      }
    }
  }

  std::string_view text_;
  std::size_t position_ = 0;
};

} // namespace

bool AccountGroup::contains(std::string_view account) const {
  auto const number = account_number(account);
  return number && in_any(included, *number) && !in_any(excluded, *number);
}

bool AccountSet::contains(std::string_view account) const {
  return std::ranges::any_of(groups, [account](AccountGroup const& group) { return group.contains(account); });
}

std::expected<AccountSet, std::string> parse_account_set(std::string_view text) {
  return SetReader(text).read();
}

} // namespace cratchit::ink2
