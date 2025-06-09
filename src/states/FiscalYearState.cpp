#include "FiscalYearState.hpp"

namespace first {
  FiscalYearState::FiscalYearState(
     StateImpl::UX ux                             
    ,FiscalPeriod fiscal_year
    ,Environment const& parent_environment_ref)
    :  StateImpl{ux}
      ,m_fiscal_year{fiscal_year}
      ,m_parent_environment_ref{parent_environment_ref} {

    this->add_option('0', {"RBD:s", RBDs_factory});

  }
} // namespace first
