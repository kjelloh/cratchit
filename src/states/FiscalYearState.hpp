#pragma once

#include "StateImpl.hpp"
#include "environment.hpp"
#include "FiscalPeriod.hpp"

namespace first {
  class FiscalYearState : public StateImpl {
  private:
    Environment const& m_parent_environment_ref;
    FiscalPeriod m_fiscal_year;

  public:
    FiscalYearState(StateImpl::UX ux,FiscalPeriod fiscal_year,Environment const& parent_environment_ref);
    virtual std::pair<std::optional<State>, Cmd> update(Msg const &msg);
  }; // struct FiscalYearState
}