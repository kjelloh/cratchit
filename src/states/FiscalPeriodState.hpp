#pragma once

#include "StateImpl.hpp"
#include "environment.hpp"
#include "FiscalPeriod.hpp"
#include "fiscal/amount/HADFramework.hpp"


namespace first {
  class FiscalPeriodState : public StateImpl {
  private:
    FiscalPeriod m_fiscal_period;
    HeadingAmountDateTransEntries m_period_hads;

  public:
    FiscalPeriodState(StateImpl::UX ux,FiscalPeriod fiscal_period,Environment const& parent_environment_ref);
    virtual std::pair<std::optional<State>, Cmd> update(Msg const &msg) override;
    virtual std::pair<std::optional<State>, Cmd> apply(cargo::HADsCargo const& cargo) const override;
    virtual Cargo get_cargo() const override;
  }; // FiscalPeriodState
}