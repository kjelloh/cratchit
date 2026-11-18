#pragma once

#include <string_view>

namespace cratchit {

// Whether an account is in the balance sheet or the income statement
enum class AccountKind { balance, result, unknown };

// By the SIE account type when given (T and S balance, K and I result), else
// by the BAS account class, its first digit: 1-2 balance, 3-8 result
AccountKind account_kind(std::string_view number, std::string_view type);

} // namespace cratchit
