#include "QuarterState.hpp"
#include "states/HADsState.hpp"
#include "VATReturnsState.hpp"
#include <format>


namespace first {
  // ----------------------------------
  QuarterState::QuarterState(StateImpl::UX ux,FiscalPeriod fiscal_period,
                             Environment const &parent_environment_ref)
    :  StateImpl{ux}
      ,m_fiscal_period{fiscal_period}
      ,m_period_hads{to_period_hads(fiscal_period,parent_environment_ref)} {

    this->ux().push_back(std::format("Fiscal Quarter: {}", fiscal_period.to_string()));

    StateFactory VATReturns_factory = [&ux,fiscal_quarter = this->m_fiscal_period]() {
      StateImpl::UX VATReturns_ux{std::format("VAT Returns for Quarter {}", fiscal_quarter.to_string())};
      return std::make_shared<VATReturnsState>(VATReturns_ux);
    };

    this->add_option('h', HADsState::option_from(this->m_period_hads,this->m_fiscal_period));
    this->add_option('v',{"VAT Returns",VATReturns_factory});
  }
}