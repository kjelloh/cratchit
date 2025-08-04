#pragma once

#include "StateImpl.hpp"
#include "fiscal/amount/HADFramework.hpp"
#include "FiscalPeriod.hpp"
#include "PeriodConstrainedContent.hpp"
#include "Mod10View.hpp"

namespace first {
  class HADsState : public StateImpl {
  public:
    using HAD = HeadingAmountDateTransEntry;
    using HADs = HeadingAmountDateTransEntries;
    using HADsSlice = PeriodSlice<HADs>;
    HADsState(HADsSlice const& hads_slice,Mod10View mod10_view);
    HADsState(HADsSlice const& hads_slice);
    HADsState(HADsState const&) = delete; // Force construct from aggregated data
    virtual StateUpdateResult update(Msg const& msg) const override;
    virtual StateImpl::UpdateOptions create_update_options() const override;
    virtual StateImpl::UX create_ux() const override;
  private:
    PeriodSlice<HADs> m_hads_slice;
    first::Mod10View m_mod10_view;    
    StateUpdateResult apply(cargo::EditedItem<HAD> const& edited_had) const;
  }; // struct HADsState
}