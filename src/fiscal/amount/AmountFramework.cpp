#include "AmountFramework.hpp"
#include "tokenize.hpp"
#include <sstream> // std::ostringstream,


namespace Key {
    Path::Path(std::vector<std::string> const &v) : m_path{v} {}
    Path::Path(std::string const &s_path, char delim)
        : m_delim{delim},
          m_path(tokenize::splits(s_path, delim,
                                  tokenize::eAllowEmptyTokens::YES)) {
      if (m_path.size() == 1 and m_path[0].size() == 0) {
        // Quick Fix that tokenize::splits will return one element of zero length for an empty string :\
        // 240623 - I do not dare to fix 'splits' itself when I do not know the effect it may have on other code...
        m_path.clear();
        std::cout << "\nKey::Path(" << std::quoted(s_path)
                  << ") PATCHED to empty path";
      }
    };
    Path Path::operator+(std::string const &key) const {
      Path result{*this};
      result.m_path.push_back(key);
      return result;
    }
    Path::operator std::string() const {
      std::ostringstream os{};
      os << *this;
      return os.str();
    }
    Path& Path::operator+=(std::string const &key) {
      m_path.push_back(key);
      // std::cout << "\noperator+= :" << *this  << " size:" << this->size();
      return *this;
    }
    Path& Path::operator--() {
      m_path.pop_back();
      // std::cout << "\noperator-- :" << *this << " size:" << this->size();
      return *this;
    }
    Path Path::parent() {
      Path result{*this};
      --result;
      // std::cout << "\nparent:" << result << " size:" << result.size();
      return result;
    }
    std::string Path::back() const { return m_path.back(); }
    std::string Path::operator[](std::size_t pos) const { return m_path[pos]; }
    std::string Path::to_string() const {
      std::ostringstream os{};
      os << *this;
      return os.str();
    }


  std::ostream &operator<<(std::ostream &os, Key::Path const &key_path) {
    int key_count{0};
    for (auto const &key : key_path) {
      if (key_count++ > 0)
        os << key_path.m_delim;
      if (false) {
        // patch to filter out unprintable characters that results from
        // incorrect character decodings NOTE: This loop will break down
        // multi-character encodings like UTF-8 and turn them into two or more
        // '?'
        for (auto ch : key) {
          if (std::isprint(ch))
            os << ch; // Filter out non printable characters (AND characters in
                      // wrong encoding, e.g., charset::ISO_8859_1 from raw file
                      // input that erroneously end up here ...)
          else
            os << "?";
        }
      } else {
        os << key; // Assume key is in runtime character encoding (ok to output
                   // as-is)
      }
      // std::cout << "\n\t[" << key_count-1 << "]:" << std::quoted(key);
    }
    return os;
  }
  std::string to_string(Key::Path const &key_path) {
    std::ostringstream os{};
    os << key_path;
    return os.str();
  }
} // namespace Key

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

// class DateRange here

Quarter to_quarter(Date const& a_period_date) {
	return {((static_cast<unsigned>(a_period_date.month())-1) / 3u)+ 1u}; // ((0..3) + 1
}
std::chrono::month to_quarter_begin(Quarter const& quarter) {
	unsigned begin_month_no = (quarter.ix-1u) * 3u + 1u; // [0..3]*3 = [0,3,6,9] + 1 = [1,4,7,10]
	return std::chrono::month{begin_month_no};
}
std::chrono::month to_quarter_end(Quarter const& quarter) {
	return (to_quarter_begin(quarter) + std::chrono::months{2});
}

DateRange to_quarter_range(Date const& a_period_date) {
// std::cout << "\nto_quarter_range: a_period_date:" << a_period_date;
	auto quarter = to_quarter(a_period_date);
	auto begin_month = to_quarter_begin(quarter);
	auto end_month = to_quarter_end(quarter);
	auto begin = Date{a_period_date.year()/begin_month/std::chrono::day{1u}};
	auto end = Date{a_period_date.year()/end_month/std::chrono::last}; // trust operator/ to adjust the day to the last day of end_month
  if (false) {
    std::cout << "\nto_quarter_range(" << a_period_date << ") --> " << begin << ".." << end;
  }
	return {begin,end};
}

DateRange to_three_months_earlier(DateRange const& quarter) {
	auto const quarter_duration = std::chrono::months{3};
  // get the year and month for the date range to return
  auto ballpark_end = quarter.end() - quarter_duration;
  // Adjust the end day to the correct one for the range end month
  auto end = ballpark_end.year() / ballpark_end.month() / std::chrono::last;
  // Note: We do not need to adjust the begin day as all month starts on day 1
	return {quarter.begin() - quarter_duration,end};
}

std::ostream& operator<<(std::ostream& os,DateRange const& dr) {
	os << dr.begin() << "..." << dr.end();
	return os;
}

// class IsPeriod here

IsPeriod to_is_period(DateRange const& period) {
	return {period};
}

std::optional<IsPeriod> to_is_period(std::string const& yyyymmdd_begin,std::string const& yyyymmdd_end) {
	std::optional<IsPeriod> result{};
	if (DateRange date_range{yyyymmdd_begin,yyyymmdd_end}) result = to_is_period(date_range);
	else {
		std::cout << "\nERROR, to_is_period failed. Invalid period " << std::quoted(yyyymmdd_begin) << " ... " << std::quoted(yyyymmdd_begin);
	}
	return result;
}

// END -- Date framework