#pragma once

#include "StateImpl.hpp"
#include "environment.hpp"
#include "FiscalPeriod.hpp"
#include "fiscal/amount/HADFramework.hpp"


namespace first {
  class FiscalYearState : public StateImpl {
  private:
    FiscalPeriod m_fiscal_period;
    HeadingAmountDateTransEntries m_period_hads;

  public:
    FiscalYearState(StateImpl::UX ux,FiscalPeriod fiscal_period,Environment const& parent_environment_ref);
    virtual std::pair<std::optional<State>, Cmd> update(Msg const &msg);
    virtual std::pair<std::optional<State>, Cmd> apply(cargo::HADsCargo const& cargo) const;
  }; // struct FiscalYearState
}