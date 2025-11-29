#pragma once

#include <optional>
#include <string>
#include <string_view>

// Swedish Bank Account Number Framework
//
// Implements parsing and validation of Swedish national bank account numbers (BBAN)
// according to the specification at:
// https://www.bankinfrastruktur.se/framtidens-betalningsinfrastruktur/iban-och-svenskt-nationellt-kontonummer
//
// Bank registry table (clearing number to bank mapping):
// https://www.bankinfrastruktur.se/framtidens-betalningsinfrastruktur/iban-och-svenskt-nationellt-kontonummer
// (See "Tabell över clearingnummer och IBAN-ID" section)

namespace BBAN {

  // Account type classification (simplified for Phase 1)
  enum class AccountType {
    Undefined,      // Cannot determine or not yet classified
    Type1Variant1,  // 11 digits total, clearing validation excludes first digit
    Type1Variant2,  // 11 digits total, full clearing validation
    Type2,          // Variable length, clearing excluded from account number
    Unknown         // Invalid or unrecognized format
  };

  // Swedish national bank account (BBAN = Basic Bank Account Number)
  struct SwedishBankAccount {
    std::string clearing_number;  // 4-5 digits (e.g., "3000" for Nordea)
    std::string account_number;   // Account number without clearing (e.g., "1234567")
    std::string bank_name;        // Bank name (e.g., "Nordea", "SEB", "Swedbank")
    AccountType type;             // Account type classification
  };

  using OptionalSwedishBankAccount = std::optional<SwedishBankAccount>;

  // Parse a suspected bank account number string into canonical BBAN format
  //
  // Accepts various input formats:
  //   "3000-1234567"      (with dash)
  //   "30001234567"       (without dash)
  //   "3000 1234567"      (with space)
  //   " 3000-1234567 "    (with whitespace)
  //
  // Returns canonical BBAN on success, nullopt on failure
  //
  // Phase 1: Basic parsing and bank lookup (no check digit validation)
  OptionalSwedishBankAccount to_swedish_bank_account(std::string_view input);

  // Format BBAN as canonical string representation
  // Returns format: "XXXX-YYYYYYY" (clearing-account with dash separator)
  std::string to_canonical_bban(SwedishBankAccount const& account);

  // Get bank name from clearing number
  // Returns bank name if found in registry, nullopt otherwise
  std::optional<std::string> to_bank_name(std::string_view clearing_number);

  // Classify account type from clearing number
  // Phase 1: Returns best-effort classification based on clearing number
  AccountType to_account_type(std::string_view clearing_number);

} // namespace BBAN
