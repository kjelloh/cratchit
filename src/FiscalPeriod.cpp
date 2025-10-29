#include "FiscalPeriod.hpp"
#include "logger/log.hpp"
#include "text/functional.hpp" // functional::text::filtered
#include <format>
#include <iomanip> // std::setfill,...

namespace first {

  // --------------------
  // BEGIN DateRange

  DateRange::DateRange(Date start, Date last)
      : m_start(start), m_last(last) {
    m_is_valid = (Days(m_start) < Days(m_last));
  }

  Date DateRange::start() const noexcept {
    return m_start;
  }

  Date DateRange::last() const noexcept {
    return m_last;
  }

  bool DateRange::contains(Date date) const noexcept {
    if (!m_is_valid)
      return false;
    auto d = Days(date);
    return d >= Days(m_start) && d <= Days(m_last);
  }

  bool DateRange::is_valid() const noexcept {
    return m_is_valid;
  }

  std::string DateRange::to_string() const {
    return std::format("{:%Y-%m-%d} to {:%Y-%m-%d}", m_start, m_last);
  }

  // END DateRange
  // --------------------

  std::ostream& operator<<(std::ostream& os, DateRange const& dr) {
    os << dr.start() << "..." << dr.last();
    return os;
  }

  DateRange to_fiscal_year_period(Year fiscal_start_year, Month fiscal_start_month) {
    // Calculate the start and last dates of the fiscal year
    Date start_date{fiscal_start_year, fiscal_start_month, std::chrono::day{1}};
    Date end_date{start_date.year() + std::chrono::years{1}, start_date.month(), std::chrono::day{0}};
    end_date = std::chrono::sys_days(end_date); // Chrono trick to normalize day 0 to the last day of previous month
    return DateRange(start_date, end_date);
  }

  DateRange to_fiscal_quarter_period(Year year,QuarterIndex quarter_ix) {
    // Calculate the start and last dates of the fiscal quarter
    Month start_month;
    switch (quarter_ix.m_ix) {
      case 1: start_month = Month{1}; break; // Q1
      case 2: start_month = Month{4}; break; // Q2
      case 3: start_month = Month{7}; break; // Q3
      case 4: start_month = Month{10}; break; // Q4
      default: throw std::invalid_argument("Invalid quarter index");
    }
    
    Date start_date{year, start_month, std::chrono::day{1}};
    Date end_date{start_date.year(), start_date.month() + std::chrono::months{3}, std::chrono::day{0}};
    end_date = std::chrono::sys_days(end_date); // Chrono trick to normalize day 0 to the last day of previous month
    return DateRange(start_date, end_date);
  }

  DateRange to_fiscal_quarter_period(Date const& a_period_date) {
    auto quarter_ix = to_quarter_index(a_period_date);
    return to_fiscal_quarter_period(a_period_date.year(),quarter_ix);
  }

  // Backward compatible with zeroth::DateRange
  DateRange to_three_months_earlier(DateRange const& date_range) {
    auto const quarter_duration = std::chrono::months{3};
    // get the year and month for the date range to return
    auto ballpark_end = date_range.end() - quarter_duration; // yymmdd - months ok
    // Adjust the last day to the correct one for the range last month
    auto last = ballpark_end.year() / ballpark_end.month() / std::chrono::last;
    // Note: We do not need to adjust the begin day as all month starts on day 1
    return {date_range.begin() - quarter_duration,last}; // yymmdd - months ok
  }

  // --------------------
  // BEGIN FiscalYear

  FiscalYear FiscalYear::to_current_fiscal_year(Month fiscal_start_month) {
    using namespace std::chrono;

    auto const today = to_today();
    Year year = today.year();
    if (today.month() < fiscal_start_month) {
      --year; // Previous fiscal year.
    }

    return FiscalYear(year, fiscal_start_month);
  }

  FiscalYear FiscalYear::to_relative_fiscal_year(int offset) const {

    // Extract fiscal year start
    const Date start_date = m_period.start();
    const Year base_year = start_date.year();
    const Month fiscal_start_month = start_date.month();

    // Offset the year
    const Year new_year = base_year + std::chrono::years{offset};

    return FiscalYear(new_year, fiscal_start_month);
  }

  // END FiscalYear
  // --------------------


  // --------------------
  // BEGIN FiscalQuarter

  FiscalQuarter FiscalQuarter::to_current_fiscal_quarter() {
    using namespace std::chrono;

    auto const today = to_today();
    auto qix = to_quarter_index(today);
    Year year = today.year();

    return FiscalQuarter(qix, year);
  }

  FiscalQuarter FiscalQuarter::to_relative_fiscal_quarter(int offset) const {

    const Date start_date = m_period.start();
    const Year base_year = start_date.year();

    QuarterIndex current_ix = to_quarter_index(start_date);

    if (!current_ix.m_valid) {
      spdlog::error("to_relative_fiscal_quarter: Invalid quarter index: {}. Returns unchanged", current_ix.m_ix);
      return *this; // TODO: Silent Nop ok?
    }

    // year 0 based quarter count
    int total_quarters = (static_cast<int>(base_year) * 4 + (current_ix.m_ix - 1)) + offset;
    Year new_year = Year{total_quarters / 4};
    QuarterIndex new_quarter = QuarterIndex{static_cast<unsigned>((total_quarters % 4) + 1)};

    return FiscalQuarter(new_quarter, new_year);
  }

  // END FiscalQuarter
  // --------------------

  std::ostream& operator<<(std::ostream& os, FiscalQuarter const& quarter) {
    os << quarter.period();
    return os;
  }

} // namespace first

// Use first::
first::OptionalDateRange to_date_range(Date const& start,Date const& last) {
  first::DateRange candidate{start,last};
  if (candidate.is_valid()) {
    return candidate;
  }
  return std::nullopt;
}

first::OptionalDateRange to_date_range(OptionalDate const& maybe_start,OptionalDate const& maybe_last) {
  if (maybe_start and maybe_last) {
    return to_date_range(maybe_start.value(),maybe_last.value());
  }
  return std::nullopt;
}

// Global namespace 

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
      std::string sDate = functional::text::filtered(sYYYYMMDD,::isdigit);
      if (sDate.size()==8) result = to_date(sDate);
    }
    // if (result) std::cout << " = " << *result;
    // else std::cout << " = null";
  }
  catch (std::exception const& e) {} // swallow silently (will result is nullopt)

  return result.and_then([](auto const& date) {
    return (date.ok()?OptionalDate(date):std::nullopt);
  });
}

Date to_today() {
  // TODO: Upgrade to correct std::chrono way when C++20 compiler support is there
  // std::cout << "\nto_today";
  std::ostringstream os{};
  auto now_timet = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::tm* now_local = localtime(&now_timet);
  return to_date(1900 + now_local->tm_year,1 + now_local->tm_mon,now_local->tm_mday);	
}

QuarterIndex to_quarter_index(Month const& month) {
  return QuarterIndex{((static_cast<unsigned>(month)-1) / 3u)+ 1u}; // ((0..3) + 1
}
QuarterIndex to_quarter_index(Date const& a_period_date) {
  return to_quarter_index(a_period_date.month());
}
std::chrono::month to_quarter_begin(QuarterIndex const& quarter_ix) {
  unsigned begin_month_no = (quarter_ix.m_ix-1u) * 3u + 1u; // [0..3]*3 = [0,3,6,9] + 1 = [1,4,7,10]
  return std::chrono::month{begin_month_no};
}
std::chrono::month to_quarter_end(QuarterIndex const& quarter_ix) {
  return (to_quarter_begin(quarter_ix) + std::chrono::months{2});
}

namespace zeroth {

  // BEGIN: first::DateRange compatible

  DateRange to_three_months_earlier(first::DateRange const& date_range) {
    return first::to_three_months_earlier(date_range);
  }

  DateRange to_quarter_range(Date const& a_period_date) {
  // std::cout << "\nto_quarter_range: a_period_date:" << a_period_date;
    // auto quarter_ix = to_quarter_index(a_period_date);
    // auto begin_month = to_quarter_begin(quarter_ix);
    // auto end_month = to_quarter_end(quarter_ix);
    // auto begin = Date{a_period_date.year()/begin_month/std::chrono::day{1u}};
    // auto last = Date{a_period_date.year()/end_month/std::chrono::last}; // trust operator/ to adjust the day to the last day of end_month
    // if (false) {
    //   spdlog::debug("to_quarter_range({}) --> {}..{}", to_string(a_period_date), to_string(begin), to_string(last));
    // }
    // return {begin,last};
    return first::to_fiscal_quarter_period(a_period_date);
  }

  // END: first::DateRange compatible


  // DateRange to_quarter_range(Date const& a_period_date) {
  // // std::cout << "\nto_quarter_range: a_period_date:" << a_period_date;
  //   auto quarter_ix = to_quarter_index(a_period_date);
  //   auto begin_month = to_quarter_begin(quarter_ix);
  //   auto end_month = to_quarter_end(quarter_ix);
  //   auto begin = Date{a_period_date.year()/begin_month/std::chrono::day{1u}};
  //   auto last = Date{a_period_date.year()/end_month/std::chrono::last}; // trust operator/ to adjust the day to the last day of end_month
  //   if (false) {
  //     spdlog::debug("to_quarter_range({}) --> {}..{}", to_string(a_period_date), to_string(begin), to_string(last));
  //   }
  //   return {begin,last};
  // }

  // DateRange to_three_months_earlier(DateRange const& quarter) {
  //   auto const quarter_duration = std::chrono::months{3};
  //   // get the year and month for the date range to return
  //   auto ballpark_end = quarter.end() - quarter_duration; // yymmdd - months ok
  //   // Adjust the last day to the correct one for the range last month
  //   auto last = ballpark_end.year() / ballpark_end.month() / std::chrono::last;
  //   // Note: We do not need to adjust the begin day as all month starts on day 1
  //   return {quarter.begin() - quarter_duration,last}; // yymmdd - months ok
  // }

  // std::ostream& operator<<(std::ostream& os,DateRange const& dr) {
  //   os << dr.begin() << "..." << dr.end();
  //   return os;
  // }

  // class IsPeriod here

  // IsPeriod to_is_period(DateRange const& period) {
  //   return {period};
  // }

  // std::optional<IsPeriod> to_is_period(std::string const& s_yyyymmdd_begin,std::string const& s_yyyymmdd_end) {
  //   std::optional<IsPeriod> result{};
  //   if (DateRange date_range{s_yyyymmdd_begin,s_yyyymmdd_end}) result = to_is_period(date_range);
  //   else {
  //     spdlog::error(R"(to_is_period failed. Invalid period "{}" ... "{}")", s_yyyymmdd_begin, s_yyyymmdd_end);
  //   }
  //   return result;
  // }

} // zeroth

