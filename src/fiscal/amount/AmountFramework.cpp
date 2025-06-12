#include "AmountFramework.hpp"
#include "tokenize.hpp"
#include <sstream> // std::ostringstream,
#include <numeric> // std::accumulate,


// BEGIN -- Date framework

std::ostream& operator<<(std::ostream& os, Date const& yyyymmdd) {
	// TODO: Remove when support for ostream << chrono::year_month_day (g++11 stdlib seems to lack support?)
	os << std::setfill('0') << std::setw(4) << static_cast<int>(yyyymmdd.year());
	os << std::setfill('0') << std::setw(2) << static_cast<unsigned>(yyyymmdd.month());
	os << std::setfill('0') << std::setw(2) << static_cast<unsigned>(yyyymmdd.day());
	return os;
}
std::string to_string(Date const& yyyymmdd) {
		std::ostringstream os{};
		os << yyyymmdd;
		return os.str();
}
Date to_date(int year,unsigned month,unsigned day) {
	return Date {
	     std::chrono::year{year}
	    ,std::chrono::month{month}
	    ,std::chrono::day{day}
	};
}
OptionalDate to_date(std::string const& sYYYYMMDD) {
	// std::cout << "\nto_date(" << sYYYYMMDD << ")";
	OptionalDate result{};
	try {
		if (sYYYYMMDD.size()==8) {
			result = to_date(
				std::stoi(sYYYYMMDD.substr(0,4))
				,static_cast<unsigned>(std::stoul(sYYYYMMDD.substr(4,2)))
				,static_cast<unsigned>(std::stoul(sYYYYMMDD.substr(6,2))));
		}
		else {
			// Handle "YYYY-MM-DD" "YYYY MM DD" etc.
			std::string sDate = filtered(sYYYYMMDD,::isdigit);
			if (sDate.size()==8) result = to_date(sDate);
		}
		// if (result) std::cout << " = " << *result;
		// else std::cout << " = null";
	}
	catch (std::exception const& e) {} // swallow silently
	return result;
}

Date to_today() {
	// TODO: Upgrade to correct std::chrono way when C++20 compiler support is there
	// std::cout << "\nto_today";
	std::ostringstream os{};
	auto now_timet = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm* now_local = localtime(&now_timet);
	return to_date(1900 + now_local->tm_year,1 + now_local->tm_mon,now_local->tm_mday);	
}

namespace WrappedCentsAmount {
  std::string to_string(CentsAmount const &cents_amount) {
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

OptionalAmount to_amount(std::string const& sAmount) {
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


