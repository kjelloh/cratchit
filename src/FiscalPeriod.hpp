#pragma once

#include <chrono>

namespace first {

  class FiscalPeriod {
  public:
    using Year = std::chrono::year;
    using Date = std::chrono::year_month_day;
    using Days = std::chrono::sys_days;

    FiscalPeriod(Date start, Date end);

    Date start() const noexcept;
    Date end() const noexcept;

    bool contains(Date date) const noexcept;
    bool is_valid() const noexcept;

  private:
    Date m_start;
    Date m_end;
    bool m_is_valid;
  };

} // namespace first
