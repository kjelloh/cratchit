#include "May2AprilState.hpp"

namespace first {
  May2AprilState::May2AprilState(
     StateImpl::UX ux                             
    ,FiscalPeriod::Year fiscal_start_year
    ,Environment const& parent_environment_ref)
    :  StateImpl{ux}
      ,m_fiscal_period{FiscalPeriod{
         std::chrono::year_month_day{fiscal_start_year, std::chrono::May, std::chrono::day{1}}
        ,std::chrono::year_month_day{fiscal_start_year + std::chrono::years{1},std::chrono::April, std::chrono::day{30}}}}
      ,m_parent_environment_ref{parent_environment_ref} {

    this->add_option('0', {"RBD:s", RBDs_factory});

  }
} // namespace first
