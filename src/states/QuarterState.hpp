#pragma once

#include "StateImpl.hpp"
#include "FiscalPeriod.hpp"

namespace first {
  class QuarterState : public StateImpl {
  public:
    QuarterState(StateImpl::UX ux,FiscalPeriod fiscal_quarter);
  private:
    FiscalPeriod m_fiscal_quarter;
  };
}