#pragma once

#include "StateImpl.hpp"
#include "environment.hpp"
#include "FiscalPeriod.hpp"
#include "fiscal/amount/HADFramework.hpp"
#include "fiscal/amount/TaggedAmountFramework.hpp"


namespace first {
  class FiscalPeriodState : public StateImpl {
  private:
    FiscalPeriod m_fiscal_period;
    HeadingAmountDateTransEntries m_period_hads;
    TaggedAmounts m_period_tagged_amounts;

  public:
    FiscalPeriodState(FiscalPeriodState const&) = delete; // Force to clone from actual data
    FiscalPeriodState(std::optional<std::string> caption,FiscalPeriod fiscal_period,HeadingAmountDateTransEntries period_hads,TaggedAmounts period_tagged_amounts);
    FiscalPeriodState(FiscalPeriod fiscal_period,Environment const& parent_environment_ref);
    virtual StateUpdateResult update(Msg const& msg) const override;
    // Cargo visit/apply double dispatch removed (cargo now message passed)
    // virtual StateUpdateResult apply(cargo::HADsCargo const& cargo) const override;
    // Cargo visit/apply double dispatch removed (cargo now message passed)
    // virtual Cargo get_cargo() const override;
    virtual StateImpl::UpdateOptions create_update_options() const override;
    virtual std::string caption() const override;

    // static StateFactory factory_from(FiscalPeriod fiscal_period,Environment const& parent_environment_ref);
    // static StateImpl::CmdOption cmd_option_from(FiscalPeriod fiscal_period,Environment const& parent_environment_ref);
    // static StateImpl::CmdOption cmd_option_from(FiscalYear fiscal_year,Environment const& parent_environment_ref);
    // static StateImpl::CmdOption cmd_option_from(FiscalQuarter fiscal_quarter,Environment const& parent_environment_ref);

  }; // FiscalPeriodState
}