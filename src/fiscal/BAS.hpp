#pragma once

#include <optional>
#include <vector>

// TODO: Continue to refactor from zeroth/main.cpp as required for the BAS namespace
namespace BAS {
  using AccountNo = unsigned int;
  using OptionalAccountNo = std::optional<AccountNo>;
  using AccountNos = std::vector<AccountNo>;
  using OptionalAccountNos = std::optional<AccountNos>;

  OptionalAccountNo to_account_no(std::string const &s);
} // namespace BAS
