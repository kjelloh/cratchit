#include "FiscalPeriod.hpp"
#include <format>

namespace first {

  FiscalPeriod::FiscalPeriod(Date start, Date end)
      : m_start(start), m_end(end) {
    m_is_valid = (Days(m_start) < Days(m_end));
  }

  FiscalPeriod::Date FiscalPeriod::start() const noexcept {
    return m_start;
  }

  FiscalPeriod::Date FiscalPeriod::end() const noexcept {
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

  std::string FiscalPeriod::to_string() const {
    return std::format("{:%Y-%m-%d} to {:%Y-%m-%d}", m_start, m_end);
  }

} // namespace first
