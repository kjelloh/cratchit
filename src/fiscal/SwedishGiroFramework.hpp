#pragma once

#include <optional>
#include <string>
#include <string_view>

// Swedish Giro Account Numbers Framework

namespace giro {

  namespace PG {
    // Swedish 'Plusgirot' id processing
    // TODO: Implement Plusgiro number validation and BBAN conversion
  }

  namespace BG {

    // Bankgiro number type classification
    enum class BankgiroType {
      Regular,    // Standard BG number (7-8 digits with dash)
      Ninety      // Special "90-number" format (90-XXXXX)
    };

    // Bankgiro account number (7-8 digits with mandatory dash separator)
    struct BankgiroNumber {
      std::string account_number;  // Full number with dash (e.g., "123-4567")
      BankgiroType type;           // Regular or Ninety
    };

    using OptionalBankgiroNumber = std::optional<BankgiroNumber>;

    // Parse a suspected Bankgiro number into validated format
    //
    // Accepts input formats WITH MANDATORY DASH:
    //   "123-4567"      ✓ (7 digits: 3 before dash, 4 after)
    //   "1234-5678"     ✓ (8 digits: 4 before dash, 4 after)
    //   "90-12345"      ✓ (90-number: special format)
    //   " 123-4567 "    ✓ (with whitespace)
    //
    // REJECTED formats:
    //   "1234567"       ✗ (missing dash)
    //   "90-00000"      ✗ (all-zero suffix)
    //   "12-34567"      ✗ (wrong dash position)
    //   "123456"        ✗ (not 7 or 8 digits)
    //
    // Validation rules:
    // - MUST contain exactly one dash '-'
    // - Total digits: MUST be exactly 7 or 8 digits
    // - Regular BG (non-90): 7 or 8 digits
    //   - 7 digits: dash MUST be after position 3 (XXX-XXXX)
    //   - 8 digits: dash MUST be after position 4 (XXXX-XXXX)
    // - 90-number: MUST be "90-XXXXX" format (7 digits)
    //   - Starts with "90"
    //   - Dash after "90"
    //   - Exactly 5 digits after dash
    //   - Suffix cannot be all zeros ("90-00000" is INVALID)
    //   - Total: 7 digits (2 + 5)
    //
    // TODO: Research and implement check digit validation for Bankgiro numbers
    //
    // Returns validated BG number on success, nullopt on failure
    OptionalBankgiroNumber to_bankgiro_number(std::string_view input);

    // Format BG number as canonical string (preserves dash separator)
    // Returns format: "XXX-XXXX" (7 digits) or "XXXX-XXXX" (8 digits)
    std::string to_canonical_bg(BankgiroNumber const& bg);

    // Check if BG number is a "90-number" (special format)
    bool is_ninety_number(BankgiroNumber const& bg);

    // Get the type of Bankgiro number
    BankgiroType get_type(BankgiroNumber const& bg);

  } // namespace BG

} // namespace giro
