#include "SwedishGiroFramework.hpp"
#include <algorithm>
#include <cctype>

namespace giro {

  namespace PG {
    // Swedish 'Plusgirot' id processing
    // TODO: Implement Plusgiro number validation and BBAN conversion
  }

  namespace BG {

    namespace detail {

      // Trim whitespace from both ends
      std::string_view trim(std::string_view s) {
        auto is_space = [](unsigned char c) { return std::isspace(c); };

        // Find first non-space character
        auto start = std::ranges::find_if_not(s, is_space);
        if (start == s.end()) return {};

        // Find last non-space character (search from end)
        auto rstart = s.rbegin();
        auto rend = s.rend();
        auto r_last_non_space = std::find_if(rstart, rend, [&](unsigned char c) {
          return !is_space(c);
        });

        // Convert reverse iterator to forward iterator
        auto end = r_last_non_space.base();

        return std::string_view(start, end);
      }

      // Count digits in a string
      size_t count_digits(std::string_view s) {
        return std::ranges::count_if(s, [](unsigned char c) {
          return std::isdigit(c);
        });
      }

      // Check if string contains only digits
      bool is_all_digits(std::string_view s) {
        return !s.empty() && std::ranges::all_of(s, [](unsigned char c) {
          return std::isdigit(c);
        });
      }

      // Check if suffix is all zeros
      bool is_all_zeros(std::string_view s) {
        return !s.empty() && std::ranges::all_of(s, [](char c) {
          return c == '0';
        });
      }

      // Find dash position in string
      std::optional<size_t> find_dash_position(std::string_view s) {
        auto it = std::ranges::find(s, '-');
        if (it == s.end()) {
          return std::nullopt;
        }
        return std::distance(s.begin(), it);
      }

      // Count occurrences of dash character
      size_t count_dashes(std::string_view s) {
        return std::ranges::count(s, '-');
      }

    } // namespace detail

    OptionalBankgiroNumber to_bankgiro_number(std::string_view input) {
      // Step 1: Trim whitespace
      std::string_view trimmed = detail::trim(input);
      if (trimmed.empty()) {
        return std::nullopt;
      }

      // Step 2: Must contain exactly one dash
      if (detail::count_dashes(trimmed) != 1) {
        return std::nullopt;
      }

      // Step 3: Find dash position
      auto maybe_dash_pos = detail::find_dash_position(trimmed);
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
      if (!detail::is_all_digits(prefix) || !detail::is_all_digits(suffix)) {
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
        if (detail::is_all_zeros(suffix)) {
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
      if (detail::is_all_zeros(suffix)) {
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

} // namespace giro
