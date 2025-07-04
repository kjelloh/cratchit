#pragma once

#include "StateImpl.hpp"
#include "fiscal/amount/HADFramework.hpp"
#include "FiscalPeriod.hpp"
#include "mod10view.hpp"

namespace first {
  class HADsState : public StateImpl {
  public:
    using HADs = HeadingAmountDateTransEntries;

    HADsState(HADs all_hads, FiscalPeriod fiscal_period,Mod10View mod10_view);
    HADsState(HADs all_hads,FiscalPeriod fiscal_period);

    virtual std::pair<std::optional<State>, Cmd> update(Msg const &msg);
    virtual Cargo get_cargo() const;

    static StateFactory factory_from(HADsState::HADs const& all_hads,FiscalPeriod fiscal_period);
    static StateImpl::CmdOption cmd_option_from(HADs const& all_hads,FiscalPeriod fiscal_period);
  private:
    HADsState::HADs m_all_hads;
    FiscalPeriod m_fiscal_period;
    first::Mod10View m_mod10_view;
  }; // struct HADsState
}