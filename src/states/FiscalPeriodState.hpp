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
    FiscalPeriodState(FiscalPeriodState const&) = delete; // Force to clone from actual data
    FiscalPeriodState(StateImpl::UX ux,FiscalPeriod fiscal_period,HeadingAmountDateTransEntries period_hads);
    FiscalPeriodState(StateImpl::UX ux,FiscalPeriod fiscal_period,Environment const& parent_environment_ref);
    virtual StateUpdateResult update(Msg const& msg) const override;
    virtual StateUpdateResult apply(cargo::HADsCargo const& cargo) const override;
    virtual Cargo get_cargo() const override;
    virtual StateImpl::UpdateOptions create_update_options() const override;

    static StateFactory factory_from(FiscalPeriod fiscal_period,Environment const& parent_environment_ref);
    // static StateImpl::CmdOption cmd_option_from(FiscalPeriod fiscal_period,Environment const& parent_environment_ref);
    // static StateImpl::CmdOption cmd_option_from(FiscalYear fiscal_year,Environment const& parent_environment_ref);
    // static StateImpl::CmdOption cmd_option_from(FiscalQuarter fiscal_quarter,Environment const& parent_environment_ref);

  }; // FiscalPeriodState
}