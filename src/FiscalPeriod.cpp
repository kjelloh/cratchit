#include "FiscalPeriod.hpp"
#include <format>
#include <spdlog/spdlog.h>

namespace first {

  FiscalPeriod::FiscalPeriod(Date start, Date end)
      : m_start(start), m_end(end) {
    m_is_valid = (Days(m_start) < Days(m_end));
  }

  Date FiscalPeriod::start() const noexcept {
    return m_start;
  }

  Date FiscalPeriod::end() const noexcept {
    return m_end;
  }

  bool FiscalPeriod::contains(Date date) const noexcept {
    if (!m_is_valid)
      return false;
    auto d = Days(date);
    return d >= Days(m_start) && d < Days(m_end);
  }

  bool FiscalPeriod::is_valid() const noexcept {
    return m_is_valid;
  }

  FiscalPeriod FiscalPeriod::to_fiscal_year(Year fiscal_start_year, Month fiscal_start_month) {
    // Calculate the start and end dates of the fiscal year
    Date start_date{fiscal_start_year, fiscal_start_month, std::chrono::day{1}};
    Date end_date{start_date.year() + std::chrono::years{1}, start_date.month(), std::chrono::day{0}};
    end_date = std::chrono::sys_days(end_date); // Chrono trick to normalize day 0 to the last day of previous month
    return FiscalPeriod(start_date, end_date);
  }

  FiscalPeriod FiscalPeriod::to_fiscal_quarter(QuarterIndex quarter_ix,Year year) {
    // Calculate the start and end dates of the fiscal quarter
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
    return FiscalPeriod(start_date, end_date);
  }

  std::string FiscalPeriod::to_string() const {
    return std::format("{:%Y-%m-%d} to {:%Y-%m-%d}", m_start, m_end);
  }

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


  FiscalQuarter FiscalQuarter::to_current_fiscal_quarter() {
    using namespace std::chrono;

    auto const today = to_today();
    auto qix = to_quarter_index(today);
    Year year = today.year();

    return FiscalQuarter(qix, year);
  }

  FiscalQuarter FiscalQuarter::to_relative_fiscal_quarter(int offset) const {
    using namespace std::chrono;

    const Date start_date = m_period.start();
    const Year base_year = start_date.year();

    QuarterIndex current_ix = to_quarter_index(start_date);
    if (!current_ix.m_valid) {
      return *this; // or handle error
    }

    // Calculate new quarter index (1-based) by wrapping offset
    int new_index = ((static_cast<int>(current_ix.m_ix) - 1 + offset) % 4 + 4) % 4 + 1;

    // Calculate year delta purely from offset
    int year_delta = offset / 4;
    if (offset < 0 && (offset % 4 != 0)) {
      year_delta -= 1;
    }

    Year new_year = base_year + years{year_delta};
    QuarterIndex new_quarter(static_cast<unsigned>(new_index));

    return FiscalQuarter(new_quarter, new_year);
  }


} // namespace first



QuarterIndex to_quarter_index(Date const& a_period_date) {
	return QuarterIndex{((static_cast<unsigned>(a_period_date.month())-1) / 3u)+ 1u}; // ((0..3) + 1
}
std::chrono::month to_quarter_begin(QuarterIndex const& quarter_ix) {
	unsigned begin_month_no = (quarter_ix.m_ix-1u) * 3u + 1u; // [0..3]*3 = [0,3,6,9] + 1 = [1,4,7,10]
	return std::chrono::month{begin_month_no};
}
std::chrono::month to_quarter_end(QuarterIndex const& quarter_ix) {
	return (to_quarter_begin(quarter_ix) + std::chrono::months{2});
}

DateRange to_quarter_range(Date const& a_period_date) {
// std::cout << "\nto_quarter_range: a_period_date:" << a_period_date;
	auto quarter_ix = to_quarter_index(a_period_date);
	auto begin_month = to_quarter_begin(quarter_ix);
	auto end_month = to_quarter_end(quarter_ix);
	auto begin = Date{a_period_date.year()/begin_month/std::chrono::day{1u}};
	auto end = Date{a_period_date.year()/end_month/std::chrono::last}; // trust operator/ to adjust the day to the last day of end_month
  if (false) {
    spdlog::debug("to_quarter_range({}) --> {}..{}", to_string(a_period_date), to_string(begin), to_string(end));
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
    spdlog::error(R"(to_is_period failed. Invalid period "{}" ... "{}")", yyyymmdd_begin, yyyymmdd_end);
	}
	return result;
}


