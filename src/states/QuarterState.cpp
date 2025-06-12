#include "QuarterState.hpp"
#include "VATReturnsState.hpp"


namespace first {
  // ----------------------------------
  QuarterState::QuarterState(StateImpl::UX ux,FiscalPeriod fiscal_quarter)
    :  StateImpl{ux}
      ,m_fiscal_quarter{fiscal_quarter} {

    this->ux().push_back(std::format("Fiscal Quarter: {}", fiscal_quarter.to_string()));

    StateFactory VATReturns_factory = [&ux,fiscal_quarter = this->m_fiscal_quarter]() {
      StateImpl::UX VATReturns_ux{std::format("VAT Returns for Quarter {}", fiscal_quarter.to_string())};
      return std::make_shared<VATReturnsState>(VATReturns_ux);
    };

    this->add_option('0',{"VAT Returns",VATReturns_factory});
  }
}