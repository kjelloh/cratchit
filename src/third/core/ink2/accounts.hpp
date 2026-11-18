#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cratchit::ink2 {

// When a group of accounts goes to a field: always, or only when the group nets to one side
enum class Condition {
  always,
  credit, // "(Om netto +)": the group nets to a credit, i.e. it adds to the year's result
  debit,  // "(Om netto -)": the group nets to a debit, i.e. it takes from the year's result
  profit, // "+ 899x": the year's result is a profit (899x carries it, so 899x nets to a debit)
  loss,   // "– 899x": the year's result is a loss (899x nets to a credit)
};

// Accounts as BAS 2023's table to INK2 writes them: four characters of digits ending in
// any number of x, e.g. 1088, 112x or 10xx, alone or as a range (151x–155x, 26xx-27xx).
// Terms are separated by ",". "(exkl. ...)" of the same, separated by "," or "och", leaves
// accounts out of the terms before it since the last exclusion: in
// "249x (exkl. 2492), 26xx-27xx, 28xx (exkl. 286x-287x), 2473, 2873" the 2873 is in.
// "(Om netto +)" or "(Om netto -)" makes the terms before it since the last condition a
// group of their own with that condition; a leading "+" or "–" (or "-") does it for all.
struct AccountGroup {
  std::vector<std::pair<int, int>> included; // first to last, both included
  std::vector<std::pair<int, int>> excluded;
  Condition condition = Condition::always;

  // Whether a four digit account number is in the group; false for any other text
  bool contains(std::string_view account) const;
};

struct AccountSet {
  std::vector<AccountGroup> groups;

  bool contains(std::string_view account) const;
};

// Error text quotes the set and where it could not be read
std::expected<AccountSet, std::string> parse_account_set(std::string_view text);

} // namespace cratchit::ink2
