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

    virtual StateUpdateResult update(Msg const &msg);
    virtual StateUpdateResult apply(cargo::EditedItemCargo<HAD> const& cargo) const;
    virtual Cargo get_cargo() const;

    static StateFactory factory_from(HADsState::HADs const& all_hads,FiscalPeriod fiscal_period);
    static StateImpl::CmdOption cmd_option_from(HADs const& all_hads,FiscalPeriod fiscal_period);
  private:
    HADsState::HADs m_all_hads;
    FiscalPeriod m_fiscal_period;
    first::Mod10View m_mod10_view;
    void refresh_ux();
  }; // struct HADsState
}