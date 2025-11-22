#include "AmountFramework.hpp"
#include "tokenize.hpp"
#include <sstream> // std::ostringstream,
#include <numeric> // std::accumulate,

// Date framework now in FiscalPeriod unit

// Amounts framework
namespace WrappedCentsAmount {
  std::string to_string(CentsAmount const& cents_amount) {
    std::ostringstream oss{};
    oss << cents_amount;
    return oss.str();
  }
}

namespace WrappedDoubleAmount {
} // namespace WrappedDoubleAmount

namespace IntCentsAmount {
} // namespace IntCentsAmount

namespace WrappedDoubleAmount {
    Amount::Amount(double value)  {
      // Convert to integer representation of cents
      long long cents = static_cast<long long>(std::round(value * 100));
      double converted_back = cents / 100.0;
      auto error = std::fabs(value - converted_back);

      // If the converted back value does not match the original, it had more than two decimal places
      // Note 240601 - The value 0.01 comes from practical testing. I still fail to understand
      //               how *100 followed by /100 can introduce such a large error?
      //               This code seems to work for now.
      //               TODO: refactor Cratchit to represent Currency values as whole integer cents to avoid the floating point precision problems!
      if (error > 0.01) {
        std::cout << "\nDESIGN_INSUFFICIENCY: Amount(" << value << ") has more than two decimal places. Error:" << error << ". Rounded it to " << converted_back;
        this->m_double_value = converted_back;
      } else {
        this->m_double_value = value; // ok    
      }
        
    }

} // namespace WrappedDoubleAmount

namespace DoubleAmount {
} // namespace DoubleAmount

/**
 * Normalize amount string to standard format for parsing
 *
 * Handles various number formats:
 * - Swedish: "1 538,50" (space thousands, comma decimal)
 * - English: "1,538.50" (comma thousands, dot decimal)
 * - Plain: "1538.50" or "1538,50"
 *
 * Returns normalized string with dot as decimal separator and no thousands separators
 */
namespace {
  std::string normalize_amount_string(std::string const& input) {
    std::string result;
    result.reserve(input.size());

    // Determine format by analyzing the string
    bool has_dot = input.find('.') != std::string::npos;
    auto comma_pos = input.rfind(',');  // Find last comma
    bool comma_is_decimal = false;

    if (comma_pos != std::string::npos) {
      // Comma is decimal separator if:
      // 1. No dot exists AND
      // 2. Comma is followed by 1-2 digits at the end (e.g., "123,5" or "123,50")
      if (!has_dot) {
        size_t digits_after_comma = 0;
        for (size_t i = comma_pos + 1; i < input.size(); ++i) {
          if (std::isdigit(static_cast<unsigned char>(input[i]))) {
            digits_after_comma++;
          } else {
            break;  // Non-digit found
          }
        }
        // Check if all remaining chars after comma are digits and count is 1-2
        bool all_digits_after = (comma_pos + 1 + digits_after_comma == input.size());
        comma_is_decimal = all_digits_after && (digits_after_comma >= 1 && digits_after_comma <= 2);
      }
      // If dot exists, comma is thousands separator (English format)
    }

    for (char ch : input) {
      if (ch == ' ' || ch == '\xA0') {
        // Skip space and non-breaking space (thousands separators)
        continue;
      }
      else if (ch == ',') {
        if (comma_is_decimal) {
          result += '.';  // Convert decimal comma to dot
        }
        // else: skip comma (thousands separator)
      }
      else {
        result += ch;
      }
    }

    return result;
  }
} // anonymous namespace

OptionalAmount to_amount(std::string const& sAmount) {
  OptionalAmount result{};

  // Handle empty or whitespace-only input
  if (sAmount.empty()) {
    return result;
  }

  // Normalize the amount string
  std::string normalized = normalize_amount_string(sAmount);

  // Handle case where normalization results in empty string
  if (normalized.empty()) {
    return result;
  }

  // Try parsing with std::stod
  try {
    size_t pos = 0;
    double value = std::stod(normalized, &pos);

    // Verify entire string was consumed (no trailing garbage)
    if (pos == normalized.size()) {
      result = Amount{value};
    }
  }
  catch (std::exception const&) {
    // Parsing failed - return empty optional
  }

  return result;
}

std::string to_string(Amount const& amount) {
  std::ostringstream oss{};
  oss << std::fixed << std::setprecision(2) << to_double(amount);
  return oss.str();
}

OptionalCentsAmount to_cents_amount(std::string const& s) {
	OptionalCentsAmount result{};
	try {
		result = CentsAmount{std::stoi(s)};
	}
	catch (...) {
		// Whine about input and failure
		std::cout << "\nThe string " << std::quoted(s) << " is not a valid cents amount (to_cents_amount returns nullopt)";
	}
	return result;
}

CentsAmount to_cents_amount(Amount const& amount) {
  // Compiler will cast double to CentsAmount::cents_value_type and use CentsAmount{cents_value_type} constructor
	return static_cast<CentsAmount>(std::round(to_double(amount)*100));
}

UnitsAndCents to_units_and_cents(CentsAmount const& cents_amount) {
	UnitsAndCents result{to_whole_part_integer(cents_amount),to_cents_part_integer(cents_amount)};
	return result;
}

Amount to_amount(UnitsAndCents const& units_and_cents) {
  return static_cast<Amount>(units_and_cents.first) + static_cast<Amount>(units_and_cents.second) / 100;
}

std::ostream& operator<<(std::ostream& os,UnitsAndCents const& units_and_cents) {
  bool is_negative = (units_and_cents.first<0) or (units_and_cents.second < 0);
  if (is_negative) os << "-";
	os << abs(units_and_cents.first) << ',' << std::setfill('0') << std::setw(2) << abs(units_and_cents.second);
	return os;
}

std::string to_string(UnitsAndCents const& units_and_cents) {
	std::ostringstream os{};
	os << units_and_cents;
	return os.str();
}


