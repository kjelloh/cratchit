#pragma once

#include <optional>
#include <string>
#include <string_view>


namespace account {

  // Swedish Giro Account Numbers Framework

  namespace giro {

    namespace PG {

      // Plusgiro account number (5-8 digits with mandatory dash before check digit)
      struct PlusgiroNumber {
        std::string number;      // The number part without check digit (4-7 digits)
        char check_digit;        // The check digit (single digit '0'-'9')

        // Format as canonical string: "XXXX-X" to "XXXXXXX-X"
        std::string to_string() const {
          return number + "-" + check_digit;
        }
      };

      using OptionalPlusgiroNumber = std::optional<PlusgiroNumber>;

      // Parse a suspected Plusgiro number into validated format
      //
      // NOTE: Some sources claim a valid PG number can be 'X-X' (one digit plus check digit)
      //       But this implementation requires at least 'XXXX-X' to identify 'reasonable' PG numbers?
      // TODO: Consider to allow the two-digit format if / when account statement parsing
      //       can handle this without miss-interpret input data as false-positive-pg-numbers?
      //       E.g., by exhausting all other account formats before matching for pg number?
      
      // Accepts input formats WITH MANDATORY DASH BEFORE CHECK DIGIT:
      //   "1234-5"        ✓ (5 digits: 4 + check digit)
      //   "12345-6"       ✓ (6 digits: 5 + check digit)
      //   "123456-7"      ✓ (7 digits: 6 + check digit)
      //   "1234567-8"     ✓ (8 digits: 7 + check digit)
      //   " 1234-5 "      ✓ (with whitespace)
      //
      // REJECTED formats:
      //   "12345"         ✗ (missing dash)
      //   "123-4567"      ✗ (wrong dash position - must be before last digit)
      //   "123"           ✗ (too few digits)
      //   "123456789-0"   ✗ (too many digits)
      //   "1234-6"        ✗ (invalid check digit)
      //   "0000-0"        ✗ (all zeros)
      //
      // Validation rules:
      // - MUST contain exactly one dash '-'
      // - Total digits: MUST be 5-8 digits (inclusive)
      // - Dash MUST be before the last digit (check digit position)
      // - Number part cannot be all zeros
      // - Check digit validated using modulo 10 algorithm
      //
      // Returns validated PG number on success, nullopt on failure
      OptionalPlusgiroNumber to_plusgiro_number(std::string_view input);

      // Format PG number as canonical string (preserves dash separator)
      // Returns format: "XXXX-X" to "XXXXXXX-X" (5-8 digits)
      std::string to_canonical_pg(PlusgiroNumber const& pg);

      // Validate check digit using modulo 10 algorithm
      // Returns true if check digit is valid
      bool validate_check_digit(std::string_view number, char check_digit);

      // Calculate expected check digit for given number using modulo 10
      // Returns the check digit character ('0'-'9')
      char calculate_check_digit(std::string_view number);

    } // namespace PG

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

} // account

