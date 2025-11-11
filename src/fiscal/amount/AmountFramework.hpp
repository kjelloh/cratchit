#pragma once

#include <string> // std::string
#include <vector> // std::vector
#include <iostream> // std::ostream,
#include <optional> // std::optional
#include <algorithm> // std::copy_if
#include <iomanip> // std::setprecision,
#include <chrono> // std::chrono::year_month_day
#include "FiscalPeriod.hpp" // fiscal Date and period classes 

// Now in FiscalPeriod unit 
// std::string filtered(std::string const& s,auto filter) {

// namespace Key now in Key unit

// All date related classes and functions are now in FiscalPerdiod unit

namespace WrappedCentsAmount {

  // A drop-in type for an integer amount in cents.
  // This implies any conversion between CentsAmount and any built in arithmetic
  // type keeps the value as a value in cents.
  class CentsAmount {
  public:
    using cents_value_type = long;
    CentsAmount() = default;
    explicit CentsAmount(CentsAmount::cents_value_type cents_value)
        : m_in_cents_value{cents_value} {}

    CentsAmount &operator+=(CentsAmount const& other) {
      this->m_in_cents_value += other.m_in_cents_value;
      return *this;
    }

    CentsAmount &operator*=(long k) {
      this->m_in_cents_value *= k;
      return *this;
    }

    CentsAmount operator+(CentsAmount const& other) const {
      CentsAmount result{*this};
      result += other;
      return result;
    }

    CentsAmount operator-() const {
      return CentsAmount{-this->m_in_cents_value};
    }

    auto operator<=>(CentsAmount const& other) const = default;

    bool operator!=(CentsAmount::cents_value_type const& cents_value) const {
      return not(this->m_in_cents_value == cents_value);
    }

    friend CentsAmount::cents_value_type to_amount_in_cents_integer(CentsAmount const& cents_amount);

  private:
    cents_value_type m_in_cents_value;
  }; // class CentsAmount

  inline CentsAmount::cents_value_type to_amount_in_cents_integer(CentsAmount const& cents_amount) {
    return cents_amount.m_in_cents_value;
  }

  inline CentsAmount abs(CentsAmount const& cents_amount) {
    return CentsAmount{std::abs(to_amount_in_cents_integer(cents_amount))};
  }

  inline CentsAmount::cents_value_type to_whole_part_integer(CentsAmount const& cents_amount) {
    return to_amount_in_cents_integer(cents_amount) / 100;
  }

  inline CentsAmount::cents_value_type to_cents_part_integer(CentsAmount const& cents_amount) {
    return to_amount_in_cents_integer(cents_amount) % 100;
  }

  inline std::ostream &operator<<(std::ostream &os, CentsAmount const& cents_amount) {
    os << to_amount_in_cents_integer(cents_amount); // keep value in integer cents
    return os;
  }

  std::string to_string(CentsAmount const& cents_amount);

} // namespace WrappedCentsAmount

using CentsAmount = WrappedCentsAmount::CentsAmount;
using OptionalCentsAmount = std::optional<CentsAmount>;

namespace IntCentsAmount {
  // Cents Amount represents e.g., 117.17 as the integer 11717
  using CentsAmount = long;

  inline CentsAmount to_whole_part_integer(CentsAmount const& cents_amount) {
    return cents_amount / 100;
  }

  inline CentsAmount to_cents_part_integer(CentsAmount const& cents_amount) {
    return cents_amount % 100;
  }

} // namespace IntCentsAmount

// The namespace for a class Amount that wraps a double as representation
namespace WrappedDoubleAmount {
  // A C++ double typically represents a floating-point number using 64 bits, 
  // following the IEEE 754 floating-point standard. 
  // This allows for approximately 15-16 decimal digits of precision or nnnnnnnnnnnnn.nnn amounts if we allow for three decimal to allow for cents rounding?

  // Amount aims to be a drop-in-replacement class for a 'using Amount = double'.
  // To enable the same code to compile and run with any of the two representations.
  // The class Amount enables more control over currency amount expressions restricted to the limits of a two decimals (cents) currency amount
  class Amount {
  public:
    Amount() = default;
    Amount(double value);

    /*

    Allow for Currency Math
    Amount + Amount = Amount
    Amount - Amount = Amount
    Amount * Amount = Not defined (NOT an amount)
    double * Amount = Amount
    Amount * Double = Amount
    Amount / double = Amount
    double / Amount = Not defined (NOT an amount)
    Amount / Amount = double ok (and also NOT an Amount)

    */

    // Amount += Amount
    Amount operator+=(const Amount& other) { 
      this->m_double_value += other.m_double_value;
      return *this;
    }

    // Amount + Amount
    Amount operator+(const Amount& other) const {
      Amount result{*this};
      result += other;
      return result;
    }

    // Amount - Amount
    Amount operator-(const Amount& other) const { return Amount(m_double_value - other.m_double_value); }
    Amount operator-() const { return Amount(-m_double_value); }
    // Amount * double
    Amount operator*(double scalar) const { return Amount(m_double_value * scalar); }
    double operator/(const Amount& other) const { return m_double_value / other.m_double_value; }

    // Amount / double
    Amount operator/(double divisor) const { return Amount(m_double_value / divisor); }

    bool operator==(Amount const& other) const {return this->m_double_value == other.m_double_value;}
    auto operator<=>(Amount const& other) const {
      return this->m_double_value <=> other.m_double_value;
    }

    friend double to_double(Amount const& amount);
    friend Amount operator*(double a, Amount const& b);
    
  private:
    double m_double_value;
  }; // class Amount

  // double + Amount
  inline Amount operator+(double a, Amount const& b) {
    return Amount{a} + b; // Do Amount + Amount
  }

  // double - Amount
  inline Amount operator-(double a, Amount const& b) {
    return Amount{a} - b; // Do Amount - Amount
  }

  // double * Amount
  inline Amount operator*(double a, Amount const& b) {
    return Amount{a} * b.m_double_value; // Do Amount * double
  }

  inline double to_double(Amount const& amount) {
    return amount.m_double_value; 
  }

  // Return Amount rounded to whole value
  inline Amount round(Amount const& amount) {
    return Amount{std::round(to_double(amount))};
  }

  // Return positive amount value (remove negative sign)
  inline Amount abs(Amount const& amount) {
    return Amount{std::abs(to_double(amount))};
  }

  // Returns Amount truncated to whole value (ignore decimal cents)
  inline Amount trunc(Amount const& amount) {
    return Amount{std::trunc(to_double(amount))};
  }

  inline std::istream& operator>>(std::istream& is, Amount& amount) {
      double double_value;
      if (is >> double_value) {
        amount = Amount{double_value};
      }
      return is;
  }

  inline std::ostream& operator<<(std::ostream& os, Amount const& amount) {
      os << std::fixed << std::setprecision(2) << to_double(amount);
      return os;
  }

} // namespace WrappedDoubleAmount

namespace std {
  template<>
  struct hash<WrappedCentsAmount::CentsAmount> {
      std::size_t operator()(WrappedCentsAmount::CentsAmount const& cents_amount) const noexcept {
          return std::hash<WrappedCentsAmount::CentsAmount::cents_value_type>{}(to_amount_in_cents_integer(cents_amount));
      }
  };
}

// namespace for Amount as a plain double (the 'original' approach)
namespace DoubleAmount {

  // using Amount= float;
  using Amount= double;

  inline double to_double(Amount const& amount) {
    return amount; 
  }
}

// Choose the Amount type to use
// using Amount = DoubleAmount::Amount;
// using DoubleAmount::to_double;
using Amount = WrappedDoubleAmount::Amount;
using OptionalAmount = std::optional<Amount>;

OptionalAmount to_amount(std::string const& sAmount);
std::string to_string(Amount const& amount);

OptionalCentsAmount to_cents_amount(std::string const& s);
CentsAmount to_cents_amount(Amount const& amount);

using UnitsAndCentsValueType = long;
using UnitsAndCents = std::pair<UnitsAndCentsValueType,UnitsAndCentsValueType>;

UnitsAndCents to_units_and_cents(CentsAmount const& cents_amount);
Amount to_amount(UnitsAndCents const& units_and_cents);
std::ostream& operator<<(std::ostream& os,UnitsAndCents const& units_and_cents);
std::string to_string(UnitsAndCents const& units_and_cents);

inline bool have_opposite_signs(Amount a1,Amount a2) {
	return ((a1 > 0) and (a2 < 0)) or ((a1 < 0) and (a2 > 0)); // Note: false also for a1==a2==0
}

inline bool are_same_and_less_than_100_cents_apart(Amount const& a1,Amount const& a2) {
	bool result = (abs(abs(a1) - abs(a2)) < 1.0);
	return result;
}


