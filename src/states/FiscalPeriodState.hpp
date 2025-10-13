#pragma once

#include "StateImpl.hpp"
#include "env/environment.hpp"
#include "FiscalPeriod.hpp"
#include "PeriodConstrainedContent.hpp"
#include "fiscal/amount/HADFramework.hpp"
#include "fiscal/amount/TaggedAmountFramework.hpp"


namespace first {
  class FiscalPeriodState : public StateImpl {
  private:
    FiscalPeriod m_fiscal_period;

    // 'Older' using separate period and container
    // HeadingAmountDateTransEntries m_period_hads;
    // TaggedAmounts m_period_tagged_amounts;

    // 'Newer' using PeriodSlice mechanism
    PeriodSlice<HeadingAmountDateTransEntries> m_hads_slice;
    PeriodSlice<TaggedAmounts> m_tas_slice;

  public:
    FiscalPeriodState(FiscalPeriodState const&) = delete; // Force to clone from actual data
    FiscalPeriodState(std::optional<std::string> caption,FiscalPeriod fiscal_period,HeadingAmountDateTransEntries period_hads,TaggedAmounts period_tagged_amounts);
    FiscalPeriodState(FiscalPeriod fiscal_period,Environment const& parent_environment_ref);
    virtual StateUpdateResult update(Msg const& msg) const override;
    virtual StateImpl::UpdateOptions create_update_options() const override;
    virtual std::string caption() const override;
  }; // FiscalPeriodState
}