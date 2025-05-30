#include "FiscalPeriod.hpp"

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
    if (!m_is_valid) return false;
    auto d = Days(date);
    return d >= Days(m_start) && d < Days(m_end);
}

bool FiscalPeriod::is_valid() const noexcept {
    return m_is_valid;
}

} // namespace first
