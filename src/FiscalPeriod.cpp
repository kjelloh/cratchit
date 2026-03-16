#include "FiscalPeriod.hpp"
#include "logger/log.hpp"
#include "text/functional.hpp" // functional::text::filtered
#include <format>
#include <iomanip> // std::setfill,...
#include <set>

namespace first {

  // --------------------
  // BEGIN DateRange

  DateRange::DateRange(Date start, Date last)
      :  m_start(start)
        ,m_last(last)
        ,m_is_valid(
              start.ok() 
          and last.ok() 
          and (Days(m_start) <= Days(m_last))) {} // single day is OK

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
    return (d >= Days(m_start)) && (d <= Days(m_last));
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

    // TODO: Consider to figure out the canonical way to find the last day
    //       of the third month from quarter start month.
    //       I failed to use year_month_day arithmetic, sys_days arithmetic
    //       or some combination of these.
    std::chrono::year_month end_yymm{start_date.year(),start_date.month() + std::chrono::months{2}};

    Date end_date = end_yymm / std::chrono::last;

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
  Date candidate{};
  if (true) {
    std::string digits{};
    std::string seps{};
    const std::set<unsigned char> valid_date_sep{
       '-'
      ,'/'
      ,' '
    };
    for (size_t i=0;i<sYYYYMMDD.size();++i) {
      auto ch = static_cast<unsigned char>(sYYYYMMDD[i]);
      if (::isdigit(ch)) digits.push_back(ch);
      if (valid_date_sep.contains(ch)) {
        if (i!=4 and i!=7) return {};
        seps.push_back(ch);
      }
    }
    if (!(digits.size()==8)) return {};
    if (!(seps.size()==0 or seps.size()==2)) return {};
    if (seps.size()==2 and seps[0] != seps[1]) return {};
    try {
      candidate = to_date(
        std::stoi(digits.substr(0,4))
        ,static_cast<unsigned>(std::stoul(digits.substr(4,2)))
        ,static_cast<unsigned>(std::stoul(digits.substr(6,2))));
    }
    catch (std::exception const& e) {
      logger::design_insufficiency("to_date Failed: Exception:{}",e.what());
    }
  }
  // Is the candidate a valid date (year,month, day makes sense?)
  if (candidate.ok()) return candidate;
  // No, failed
  return {};
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

// TODO: Consider ways to put to_string overload in same namespace as type / 2026019
//       For now this is impossible (See thinking.md 'How can I make all free function to_string overloads to be found by the compiler?')
//       1. Date is an alias to std::chrono::year_month_day
//       2. Thus ADL will NOT find to_string(date) in e.g. namespace first (where the alias is defined)
//       3. Thus to_string(Date) is in global namespace for now (compiler will find it by default)
//       4. BUT then we can NOT put to_string(FiscalPeriod) in namespace first!
//       5. Because if we do - then compiler name lookup will use local namespace to_string and ADL found to_string
//          And local namespace to_string has NO overload for Date.
//          And ADL will look in std::chrono (also NO to_string(Date))
//       Possible 'better' ways are:
//       1. Refactor all code to use native type std::chrono::year_month_day 'to string' options (good enough?)
//       2. Make a 'strong type' around std::chrono::year_month_day to make ADL work into e.g. namespave first.
//       ...
std::string to_string(FiscalPeriod const& fiscal_period) {
  return fiscal_period.to_string();
}
std::string to_string(std::optional<FiscalPeriod> const& maybe_fiscal_period) {
  return maybe_fiscal_period
    .transform([](auto const& date_range){return date_range.to_string();})
    .value_or("anonymous");
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

