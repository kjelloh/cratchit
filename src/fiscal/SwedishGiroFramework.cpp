#include "SwedishGiroFramework.hpp"
#include "text/functional.hpp" // trim,...
#include <algorithm>
#include <cctype>

namespace giro {

  namespace BG {

    namespace detail {
    } // namespace detail

    OptionalBankgiroNumber to_bankgiro_number(std::string_view input) {
      // Step 1: Trim whitespace
      std::string_view trimmed = text::functional::trim(input);
      if (trimmed.empty()) {
        return std::nullopt;
      }

      // Step 2: Must contain exactly one dash
      if (text::functional::count_dashes(trimmed) != 1) {
        return std::nullopt;
      }

      // Step 3: Find dash position
      auto maybe_dash_pos = text::functional::find_dash_position(trimmed);
      if (!maybe_dash_pos) {
        return std::nullopt;  // Should not happen given count_dashes check
      }
      size_t dash_pos = *maybe_dash_pos;

      // Step 4: Extract parts before and after dash
      std::string_view prefix = trimmed.substr(0, dash_pos);
      std::string_view suffix = trimmed.substr(dash_pos + 1);

      // Step 5: Both parts must be non-empty and contain only digits
      if (prefix.empty() || suffix.empty()) {
        return std::nullopt;
      }
      if (!text::functional::is_all_digits(prefix) || !text::functional::is_all_digits(suffix)) {
        return std::nullopt;
      }

      // Step 6: Total must be exactly 7 or 8 digits
      size_t total_digits = prefix.size() + suffix.size();
      if (total_digits != 7 && total_digits != 8) {
        return std::nullopt;
      }

      // Step 7: Check for 90-number format
      if (prefix == "90") {
        // 90-number validation
        if (suffix.size() != 5) {
          return std::nullopt;  // Must be exactly 5 digits after "90-"
        }
        if (text::functional::is_all_zeros(suffix)) {
          return std::nullopt;  // "90-00000" is invalid
        }

        // Valid 90-number
        return BankgiroNumber{
          .account_number = std::string(trimmed),
          .type = BankgiroType::Ninety
        };
      }

      // Step 8: Regular BG number validation (non-90)
      if (total_digits == 7) {
        // 7 digits: dash must be after position 3 (XXX-XXXX)
        if (prefix.size() != 3 || suffix.size() != 4) {
          return std::nullopt;
        }
      } else if (total_digits == 8) {
        // 8 digits: dash must be after position 4 (XXXX-XXXX)
        if (prefix.size() != 4 || suffix.size() != 4) {
          return std::nullopt;
        }
      }

      // Step 9: Reject all-zero suffix for regular numbers
      if (text::functional::is_all_zeros(suffix)) {
        return std::nullopt;
      }

      // Step 10: Valid regular BG number
      return BankgiroNumber{
        .account_number = std::string(trimmed),
        .type = BankgiroType::Regular
      };
    }

    std::string to_canonical_bg(BankgiroNumber const& bg) {
      return bg.account_number;
    }

    bool is_ninety_number(BankgiroNumber const& bg) {
      return bg.type == BankgiroType::Ninety;
    }

    BankgiroType get_type(BankgiroNumber const& bg) {
      return bg.type;
    }

  } // namespace BG

  namespace PG {

    namespace detail {

      // Calculate modulo 10 check digit (Luhn-like algorithm)
      // Process digits from right to left, alternating multiply by 2 and 1
      char calculate_mod10_check_digit(std::string_view digits) {
        if (digits.empty()) return '0';

        int sum = 0;
        bool multiply_by_two = true;  // Start with rightmost digit * 2

        // Process from right to left
        for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
          int digit = *it - '0';

          if (multiply_by_two) {
            digit *= 2;
            // If result is two digits, add them (e.g., 16 -> 1+6=7)
            if (digit > 9) {
              digit = (digit / 10) + (digit % 10);
            }
          }

          sum += digit;
          multiply_by_two = !multiply_by_two;
        }

        // Check digit is (10 - (sum % 10)) % 10
        int check = (10 - (sum % 10)) % 10;
        return '0' + check;
      }

    } // namespace detail

    char calculate_check_digit(std::string_view number) {
      return detail::calculate_mod10_check_digit(number);
    }

    bool validate_check_digit(std::string_view number, char check_digit) {
      char expected = calculate_check_digit(number);
      return expected == check_digit;
    }

    OptionalPlusgiroNumber to_plusgiro_number(std::string_view input) {
      // Step 1: Trim whitespace
      std::string_view trimmed = text::functional::trim(input);
      if (trimmed.empty()) {
        return std::nullopt;
      }

      // Step 2: Must contain exactly one dash
      if (text::functional::count_dashes(trimmed) != 1) {
        return std::nullopt;
      }

      // Step 3: Find dash position
      auto maybe_dash_pos = text::functional::find_dash_position(trimmed);
      if (!maybe_dash_pos) {
        return std::nullopt;
      }
      size_t dash_pos = *maybe_dash_pos;

      // Step 4: Extract parts before and after dash
      std::string_view number_part = trimmed.substr(0, dash_pos);
      std::string_view check_part = trimmed.substr(dash_pos + 1);

      // Step 5: Both parts must be non-empty and contain only digits
      if (number_part.empty() || check_part.empty()) {
        return std::nullopt;
      }
      if (!text::functional::is_all_digits(number_part) || !text::functional::is_all_digits(check_part)) {
        return std::nullopt;
      }

      // Step 6: Check part must be exactly 1 digit (the check digit)
      if (check_part.size() != 1) {
        return std::nullopt;
      }

      // Step 7: Total must be 5-8 digits (4-7 number + 1 check)
      size_t total_digits = number_part.size() + check_part.size();
      if (total_digits < 5 || total_digits > 8) {
        return std::nullopt;
      }

      // Step 8: Reject all-zeros number part
      if (text::functional::is_all_zeros(number_part)) {
        return std::nullopt;
      }

      // Step 9: Validate check digit using modulo 10
      char check_digit = check_part[0];
      if (!validate_check_digit(number_part, check_digit)) {
        return std::nullopt;
      }

      // Valid Plusgiro number
      return PlusgiroNumber{
        .number = std::string(number_part),
        .check_digit = check_digit
      };
    }

    std::string to_canonical_pg(PlusgiroNumber const& pg) {
      return pg.to_string();
    }

  } // namespace PG

} // namespace giro
