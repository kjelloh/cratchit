#pragma once

#include "StateImpl.hpp"
#include "fiscal/amount/HADFramework.hpp"
#include "FiscalPeriod.hpp"
#include "mod10view.hpp"

namespace first {
  class HADsState : public StateImpl {
  public:
    using HAD = HeadingAmountDateTransEntry;
    using HADs = HeadingAmountDateTransEntries;

    HADsState(HADs all_hads, FiscalPeriod fiscal_period,Mod10View mod10_view);
    HADsState(HADs all_hads,FiscalPeriod fiscal_period);
    HADsState(HADsState const&) = delete; // Force construct from data = ux ok

    virtual StateUpdateResult update(Msg const& msg) const override;

    virtual StateImpl::UpdateOptions create_update_options() const override;
    virtual StateImpl::UX create_ux() const override;

  private:
    HADsState::HADs m_all_hads;
    FiscalPeriod m_fiscal_period;
    first::Mod10View m_mod10_view;
    
    StateUpdateResult apply(cargo::EditedItem<HAD> const& edited_had) const;
  }; // struct HADsState
}