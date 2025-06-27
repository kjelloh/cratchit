#pragma once

#include "StateImpl.hpp"
#include "FiscalPeriod.hpp"
#include "environment.hpp"

namespace first {
  class QuarterState : public StateImpl {
  public:
    QuarterState(StateImpl::UX ux,FiscalPeriod fiscal_period,Environment const &parent_environment_ref);
  private:
    FiscalPeriod m_fiscal_period;
    HeadingAmountDateTransEntries m_period_hads;
  };
}