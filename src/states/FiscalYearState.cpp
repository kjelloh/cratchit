#include "FiscalYearState.hpp"
#include "RBDsState.hpp"
#include "fiscal/amount/HADFramework.hpp"
#include <spdlog/spdlog.h>

namespace first {
  FiscalYearState::FiscalYearState(
     StateImpl::UX ux                             
    ,FiscalPeriod fiscal_year
    ,Environment const& parent_environment_ref)
    :  StateImpl{ux}
      ,m_fiscal_year{fiscal_year}
      ,m_parent_environment_ref{parent_environment_ref} {

    try {
        auto hads = hads_from_environment(m_parent_environment_ref);
        // TODO: Use hads to expose HAD options to the user

        StateFactory RBDs_factory = []() {
        auto all_rbds = RBDsState::RBDs{
            "RBD #0",  "RBD #1",  "RBD #2",  "RBD #3",  "RBD #4",  "RBD #5",
            "RBD #6",  "RBD #7",  "RBD #8",  "RBD #9",  "RBD #10", "RBD #11",
            "RBD #12", "RBD #13", "RBD #14", "RBD #15", "RBD #16", "RBD #17",
            "RBD #18", "RBD #19", "RBD #20", "RBD #21", "RBD #22", "RBD #23"};
        return std::make_shared<RBDsState>(all_rbds);
        };

        this->add_option('0', {"RBD:s", RBDs_factory});
    }
    catch (std::exception const &e) {
        spdlog::error("Error initializing FiscalYearState: {}", e.what());
    }


  }
} // namespace first
