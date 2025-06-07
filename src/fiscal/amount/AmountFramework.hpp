#pragma once

#include <iostream> // std::ostream,
#include <sstream> // std::ostringstream,
#include <algorithm> // std::copy_if
#include <numeric> // std::accumulate,
#include <iomanip> // std::setprecision,
#include <chrono> // std::chrono::year_month_day
#include <vector> // std::vector
#include <string> // std::string

std::string filtered(std::string const& s,auto filter) {
	std::string result{};;
	std::copy_if(s.begin(),s.end(),std::back_inserter(result),filter);
	return result;
}

namespace Key {
  class Path {
  public:
    auto begin() const { return m_path.begin(); }
    auto end() const { return m_path.end(); }
    Path() = default;
    Path(Path const &other) = default;
    Path(std::vector<std::string> const &v);
    Path(std::string const &s_path, char delim = '^');
    auto size() const { return m_path.size(); }
    Path operator+(std::string const &key) const;
    operator std::string() const;
    Path &operator+=(std::string const &key);
    Path &operator--();
    Path parent();
    std::string back() const;
    std::string operator[](std::size_t pos) const;
    friend std::ostream &operator<<(std::ostream &os, Path const &key_path);
    std::string to_string() const;

  private:
    std::vector<std::string> m_path{};
    char m_delim{'^'};
  }; // class Path

  using Paths = std::vector<Path>;

  std::ostream &operator<<(std::ostream &os, Key::Path const &key_path);
  std::string to_string(Key::Path const &key_path);
} // namespace Key

// BEGIN -- Date framework
using Date = std::chrono::year_month_day;
using OptionalDate = std::optional<Date>;

std::ostream& operator<<(std::ostream& os, Date const& yyyymmdd);
std::string to_string(Date const& yyyymmdd);
Date to_date(int year,unsigned month,unsigned day);
OptionalDate to_date(std::string const& sYYYYMMDD);
Date to_today();

class DateRange {
public:
	DateRange(Date const& begin,Date const& end) : m_begin{begin},m_end{end} {}
	DateRange(std::string const& yyyymmdd_begin,std::string const& yyyymmdd_end) {
		OptionalDate begin{to_date(yyyymmdd_begin)};
		OptionalDate end{to_date(yyyymmdd_end)};
		if (begin and end) {
			m_valid = true;
			m_begin = *begin;
			m_end = *end;
		}
	}
	Date begin() const {return m_begin;}
	Date end() const {return m_end;}
	bool contains(Date const& date) const { return begin() <= date and date <= end();}
	operator bool() const {return m_valid;}
private:
	bool m_valid{};
	Date m_begin{};
	Date m_end{};
};
using OptionalDateRange = std::optional<DateRange>;

struct Quarter {
	unsigned ix;
};

Quarter to_quarter(Date const& a_period_date);
std::chrono::month to_quarter_begin(Quarter const& quarter);
std::chrono::month to_quarter_end(Quarter const& quarter);
DateRange to_quarter_range(Date const& a_period_date);
DateRange to_three_months_earlier(DateRange const& quarter);
std::ostream& operator<<(std::ostream& os,DateRange const& dr);

struct IsPeriod {
	DateRange period;
	bool operator()(Date const& date) const {
		return period.contains(date);
	}
};

IsPeriod to_is_period(DateRange const& period);
std::optional<IsPeriod> to_is_period(std::string const& yyyymmdd_begin,std::string const& yyyymmdd_end);

// END -- Date framework


namespace WrappedCentsAmount {

  // A drop-in type for an integer amount in cents.
  // This implies any conversion between CentsAmount and any built in arithmetic
  // type keeps the value as a value in cents.
  class CentsAmount {
  public:
    using cents_value_type = int;
    CentsAmount() = default;
    explicit CentsAmount(CentsAmount::cents_value_type cents_value)
        : m_in_cents_value{cents_value} {}

    CentsAmount &operator+=(CentsAmount const &other) {
      this->m_in_cents_value += other.m_in_cents_value;
      return *this;
    }

    CentsAmount operator+(CentsAmount const &other) {
      CentsAmount result{*this};
      result += other;
      return result;
    }

    auto operator<=>(CentsAmount const &other) const = default;

    bool operator==(CentsAmount const &other) const {
      return this->m_in_cents_value == other.m_in_cents_value;
    }
    bool operator!=(CentsAmount const &other) const {
      return not(*this == other);
    }
    bool operator!=(CentsAmount::cents_value_type const &cents_value) const {
      return not(this->m_in_cents_value == cents_value);
    }

    friend CentsAmount::cents_value_type
    to_amount_in_cents_integer(CentsAmount const &cents_amount);

  private:
    cents_value_type m_in_cents_value;
  }; // class CentsAmount

  inline CentsAmount::cents_value_type to_amount_in_cents_integer(CentsAmount const &cents_amount) {
    return cents_amount.m_in_cents_value;
  }

  inline CentsAmount abs(CentsAmount const &cents_amount) {
    return CentsAmount{std::abs(to_amount_in_cents_integer(cents_amount))};
  }

  inline CentsAmount::cents_value_type to_whole_part_integer(CentsAmount const &cents_amount) {
    return to_amount_in_cents_integer(cents_amount) / 100;
  }

  inline CentsAmount::cents_value_type to_cents_part_integer(CentsAmount const &cents_amount) {
    return to_amount_in_cents_integer(cents_amount) % 100;
  }

  inline std::ostream &operator<<(std::ostream &os, CentsAmount const &cents_amount) {
    os << to_amount_in_cents_integer(
        cents_amount); // keep value in integer cents
    return os;
  }

  inline std::string to_string(CentsAmount const &cents_amount) {
    std::ostringstream oss{};
    oss << cents_amount;
    return oss.str();
  }
} // namespace WrappedCentsAmount

using CentsAmount = WrappedCentsAmount::CentsAmount;
using OptionalCentsAmount = std::optional<CentsAmount>;

namespace IntCentsAmount {
  // Cents Amount represents e.g., 117.17 as the integer 11717
  using CentsAmount = int;

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
    Amount(double value)  {
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
    friend Amount operator*(double a, Amount const &b);
    
  private:
    double m_double_value;
  }; // class Amount

  // double + Amount
  inline Amount operator+(double a, Amount const &b) {
    return Amount{a} + b; // Do Amount + Amount
  }

  // double - Amount
  inline Amount operator-(double a, Amount const &b) {
    return Amount{a} - b; // Do Amount - Amount
  }

  // double * Amount
  inline Amount operator*(double a, Amount const &b) {
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

inline OptionalAmount to_amount(std::string const& sAmount) {
	// std::cout << "\nto_amount " << std::quoted(sAmount);
	OptionalAmount result{};
	Amount amount{};
	std::istringstream is{sAmount};
	if (auto pos = sAmount.find(','); pos != std::string::npos) {
		// Handle 123,45 ==> 123.45
		result = to_amount(std::accumulate(sAmount.begin(),sAmount.end(),std::string{},[](auto acc,char ch){
			acc += (ch==',')?'.':ch;
			return acc;
		}));
	}
	else if (is >> amount) {
		result = amount;
	}
	else {
		// handle integer (no decimal point)
		try {
			auto int_amount = std::stoi(sAmount);
			result = static_cast<Amount>(int_amount);
		}
		catch (std::exception const& e) { /* failed - do nothing */}
	}
	// if (result) std::cout << "\nresult " << *result;
	return result;
}

inline std::string to_string(Amount const& amount) {
  std::ostringstream oss{};
  oss << std::fixed << std::setprecision(2) << to_double(amount);
  return oss.str();
}

inline OptionalCentsAmount to_cents_amount(std::string const& s) {
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

inline CentsAmount to_cents_amount(Amount const& amount) {
  // Compiler will cast double to CentsAmount::cents_value_type and use CentsAmount{cents_value_type} constructor
	return static_cast<CentsAmount>(std::round(to_double(amount)*100));
}

using UnitsAndCentsValueType = int;
using UnitsAndCents = std::pair<UnitsAndCentsValueType,UnitsAndCentsValueType>;

inline UnitsAndCents to_units_and_cents(CentsAmount const& cents_amount) {
	UnitsAndCents result{to_whole_part_integer(cents_amount),to_cents_part_integer(cents_amount)};
	return result;
}

inline Amount to_amount(UnitsAndCents const& units_and_cents) {
  return static_cast<Amount>(units_and_cents.first) + static_cast<Amount>(units_and_cents.second) / 100;
}

inline std::ostream& operator<<(std::ostream& os,UnitsAndCents const& units_and_cents) {
  bool is_negative = (units_and_cents.first<0) or (units_and_cents.second < 0);
  if (is_negative) os << "-";
	os << abs(units_and_cents.first) << ',' << std::setfill('0') << std::setw(2) << abs(units_and_cents.second);
	return os;
}

inline std::string to_string(UnitsAndCents const& units_and_cents) {
	std::ostringstream os{};
	os << units_and_cents;
	return os.str();
}
