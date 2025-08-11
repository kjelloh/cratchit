#pragma once

#include <chrono>

namespace first {

  using Date = std::chrono::year_month_day;
  using Days = std::chrono::sys_days;
  using Year = std::chrono::year;
  using Month = std::chrono::month;

  struct QuarterIndex {
    unsigned m_ix; // Must be 1..4
    bool m_valid;

    explicit QuarterIndex(unsigned ix)
        : m_ix(ix), m_valid(ix >= 1 && ix <= 4) {}
  };

  class FiscalPeriod {
  public:

    FiscalPeriod(Date start, Date end);

    Date start() const noexcept;
    Date end() const noexcept;

    bool contains(Date date) const noexcept;
    bool is_valid() const noexcept;

    static FiscalPeriod to_fiscal_year(Year fiscal_start_year, Month fiscal_start_month);
    static FiscalPeriod to_fiscal_quarter(QuarterIndex quarter_ix, Year year);

    std::string to_string() const;

  private:
    Date m_start;
    Date m_end;
    bool m_is_valid;
  };

  class FiscalYear {
  public:
    FiscalYear(Year year, Month fiscal_start_month)
      : m_period(FiscalPeriod::to_fiscal_year(year, fiscal_start_month)) {}

    const FiscalPeriod& period() const noexcept { return m_period; }

    Date start() const noexcept { return m_period.start(); }
    Date end() const noexcept { return m_period.end(); }
    bool contains(Date d) const noexcept { return m_period.contains(d); }
    bool is_valid() const noexcept { return m_period.is_valid(); }

    std::string to_string() const { return m_period.to_string(); }

    static FiscalYear to_current_fiscal_year(Month fiscal_start_month);
    FiscalYear to_relative_fiscal_year(int offset) const;

  private:
    FiscalPeriod m_period;
  };

  class FiscalQuarter {
  public:
    FiscalQuarter(QuarterIndex quarter_ix, Year year)
      : m_period(FiscalPeriod::to_fiscal_quarter(quarter_ix, year)) {}

    const FiscalPeriod& period() const noexcept { return m_period; }

    Date start() const noexcept { return m_period.start(); }
    Date end() const noexcept { return m_period.end(); }
    bool contains(Date d) const noexcept { return m_period.contains(d); }
    bool is_valid() const noexcept { return m_period.is_valid(); }

    std::string to_string() const { return m_period.to_string(); }

    static FiscalQuarter to_current_fiscal_quarter();
    FiscalQuarter to_relative_fiscal_quarter(int offset) const;

  private:
    FiscalPeriod m_period;
  };

} // namespace first

using Date = first::Date;
using Year = first::Year;
using Month = first::Month;
using QuarterIndex = first::QuarterIndex;
using FiscalPeriod = first::FiscalPeriod;
using FiscalYear = first::FiscalYear;

// global namespace
using OptionalDate = std::optional<Date>;
std::ostream& operator<<(std::ostream& os, Date const& yyyymmdd);
std::string to_string(Date const& yyyymmdd);
Date to_date(int year,unsigned month,unsigned day);
OptionalDate to_date(std::string const& sYYYYMMDD);
Date to_today();
QuarterIndex to_quarter_index(Date const& a_period_date);
Month to_quarter_begin(QuarterIndex const& quarter_ix);
Month to_quarter_end(QuarterIndex const& quarter_ix);

namespace zeroth {

  // --------------------------------------------------------------------------------

  // BEGIN -- Date framework (from original zeroth variant)

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
  }; // DateRange

  using OptionalDateRange = std::optional<DateRange>;

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
  
} // zeroth


// END -- Date framework

