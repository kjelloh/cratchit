#include "fiscal/BBANFramework.hpp"
#include <algorithm>
#include <cctype>
#include <ranges>

namespace BBAN {

namespace detail {

  // Bank registry: clearing number ranges to bank names (Phase 1: major banks only)
  struct ClearingRange {
    unsigned int start;
    unsigned int end;
    std::string_view bank_name;
    AccountType account_type;
  };

  // Hard-coded bank registry for major Swedish banks
  // Source: https://www.bankinfrastruktur.se/framtidens-betalningsinfrastruktur/iban-och-svenskt-nationellt-kontonummer
  constexpr ClearingRange BANK_REGISTRY[] = {
    // Nordea
    {1100, 1199, "Nordea", AccountType::Type1Variant1},
    {1400, 2099, "Nordea", AccountType::Type1Variant1},
    {3000, 3299, "Nordea", AccountType::Type1Variant1},
    {3301, 3399, "Nordea", AccountType::Type1Variant1},
    {3410, 4999, "Nordea", AccountType::Type1Variant1},

    // Danske Bank
    {1200, 1399, "Danske Bank", AccountType::Type1Variant1},

    // SEB
    {5000, 5999, "SEB", AccountType::Type1Variant1},
    {9120, 9124, "SEB", AccountType::Type1Variant1},
    {9130, 9149, "SEB", AccountType::Type1Variant1},

    // Handelsbanken
    {6000, 6999, "Handelsbanken", AccountType::Type1Variant2},

    // Swedbank
    {7000, 7999, "Swedbank", AccountType::Type1Variant1},
    {8000, 8999, "Swedbank", AccountType::Type2},

    // Länsförsäkringar Bank
    {9020, 9029, "Länsförsäkringar Bank", AccountType::Type1Variant1},
    {9060, 9069, "Länsförsäkringar Bank", AccountType::Type1Variant1},
  };

  // Lookup bank information by clearing number
  std::optional<ClearingRange const*> lookup_bank(unsigned int clearing) {
    for (auto const& range : BANK_REGISTRY) {
      if (clearing >= range.start && clearing <= range.end) {
        return &range;
      }
    }
    return std::nullopt;
  }

  // Convert string to unsigned int, returns nullopt on failure
  std::optional<unsigned int> to_uint(std::string_view s) {
    if (s.empty()) return std::nullopt;

    unsigned int result = 0;
    for (char c : s) {
      if (!std::isdigit(static_cast<unsigned char>(c))) {
        return std::nullopt;
      }
      result = result * 10 + (c - '0');
    }
    return result;
  }

  // Trim whitespace from both ends
  std::string_view trim(std::string_view s) {
    auto is_space = [](unsigned char c) { return std::isspace(c); };

    auto start = std::ranges::find_if_not(s, is_space);
    if (start == s.end()) return {};

    auto end = std::ranges::find_if_not(s | std::views::reverse, is_space).base();

    return std::string_view(start, end);
  }

  // Remove all non-digit characters from string
  std::string remove_non_digits(std::string_view s) {
    std::string result;
    result.reserve(s.size());
    for (char c : s) {
      if (std::isdigit(static_cast<unsigned char>(c))) {
        result += c;
      }
    }
    return result;
  }

} // namespace detail

OptionalSwedishBankAccount to_swedish_bank_account(std::string_view input) {
  // Step 1: Trim whitespace
  std::string_view trimmed = detail::trim(input);
  if (trimmed.empty()) {
    return std::nullopt;
  }

  // Step 2: Validate that first 4 characters (after trimming) start with digits
  // This prevents accepting inputs like "ABC-1234567" where clearing is non-numeric
  if (trimmed.size() < 4) {
    return std::nullopt;
  }
  for (size_t i = 0; i < 4; ++i) {
    char c = trimmed[i];
    if (!std::isdigit(static_cast<unsigned char>(c))) {
      return std::nullopt;
    }
  }

  // Step 3: Extract digits only (handles "3000-1234567", "3000 1234567", "30001234567")
  std::string digits = detail::remove_non_digits(trimmed);

  // Step 4: Validate minimum length (clearing 4 digits + at least 1 account digit)
  if (digits.size() < 5) {
    return std::nullopt;
  }

  // Step 5: Maximum length check (4-5 digit clearing + max 12 digit account = 17)
  if (digits.size() > 17) {
    return std::nullopt;
  }

  // Step 6: Try 4-digit clearing first (most common)
  std::string clearing_4 = digits.substr(0, 4);
  auto clearing_num = detail::to_uint(clearing_4);

  if (!clearing_num) {
    return std::nullopt;
  }

  // Step 7: Lookup bank information
  auto maybe_bank_info = detail::lookup_bank(*clearing_num);
  if (!maybe_bank_info) {
    return std::nullopt;
  }

  auto const& bank_info = **maybe_bank_info;

  // Step 8: Extract account number (everything after clearing)
  std::string account_num = digits.substr(4);

  // Step 9: Return parsed account
  return SwedishBankAccount{
    .clearing_number = clearing_4,
    .account_number = account_num,
    .bank_name = std::string(bank_info.bank_name),
    .type = bank_info.account_type
  };
}

std::string to_canonical_bban(SwedishBankAccount const& account) {
  return account.clearing_number + "-" + account.account_number;
}

std::optional<std::string> to_bank_name(std::string_view clearing_number) {
  auto clearing_num = detail::to_uint(clearing_number);
  if (!clearing_num) {
    return std::nullopt;
  }

  auto maybe_bank_info = detail::lookup_bank(*clearing_num);
  if (!maybe_bank_info) {
    return std::nullopt;
  }

  return std::string((*maybe_bank_info)->bank_name);
}

AccountType to_account_type(std::string_view clearing_number) {
  auto clearing_num = detail::to_uint(clearing_number);
  if (!clearing_num) {
    return AccountType::Undefined;
  }

  auto maybe_bank_info = detail::lookup_bank(*clearing_num);
  if (!maybe_bank_info) {
    return AccountType::Unknown;
  }

  return (*maybe_bank_info)->account_type;
}

} // namespace BBAN
