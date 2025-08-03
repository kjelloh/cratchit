#pragma once

#include "FiscalPeriod.hpp"

template <class T>
class PeriodPairedT {
public:
  PeriodPairedT(FiscalPeriod p, T const& t)
    :  m_p{p}
      ,m_t{t} {}

  auto const& period() const {return m_p;}
  auto const& content() const {return m_t;}
private:
  FiscalPeriod m_p;
  T m_t;
}; // PeriodSlice

template <class T>
class PeriodSlice {
public:
  using value_type = typename T::value_type;
  using to_date_projection = std::function<Date(value_type)>;
  PeriodSlice(FiscalPeriod p, T const& t) :  m_period_paired{p,t} {}

  auto const& period() const {return m_period_paired.period();}
  auto const& content() const {return m_period_paired.content();}
private:
  PeriodPairedT<T> m_period_paired;
}; // PeriodSlice

template <class T>
PeriodPairedT<T> make_period_paired(FiscalPeriod period,T const& content) {
  return PeriodPairedT<T>{period,content};
}

template <class T>
PeriodSlice<T> make_period_sliced(FiscalPeriod period,T const& container,typename PeriodSlice<T>::to_date_projection to_date) {
  T slice{};
  for (auto const& member : container) {
    if (period.contains(to_date(member))) slice.emplace_back(member);
  }
  return PeriodSlice<T>{period,slice};
}


