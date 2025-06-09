#pragma once

#include <chrono>

namespace first {

    class FiscalPeriod {
    public:
        using Date = std::chrono::year_month_day;
        using Days = std::chrono::sys_days;
        using Year = std::chrono::year;
        using Month = std::chrono::month;

        FiscalPeriod(Date start, Date end);

        Date start() const noexcept;
        Date end() const noexcept;

        bool contains(Date date) const noexcept;
        bool is_valid() const noexcept;

        static FiscalPeriod to_fiscal_year(Year fiscal_start_year,Month fiscal_start_month);

        std::string to_string() const;

    private:
        Date m_start;
        Date m_end;
        bool m_is_valid;
    };
} // namespace first

// BEGIN -- Date framework (from original zeroth variant)
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

