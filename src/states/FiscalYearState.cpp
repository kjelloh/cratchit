#include "FiscalYearState.hpp"
#include "HADsState.hpp"
#include "fiscal/amount/HADFramework.hpp"
#include <spdlog/spdlog.h>

namespace first {
  FiscalYearState::FiscalYearState(
      StateImpl::UX ux, FiscalPeriod fiscal_year, Environment const &parent_environment_ref)
      : StateImpl{ux}, m_fiscal_year{fiscal_year}, m_parent_environment_ref{parent_environment_ref} {


    StateFactory HADs_factory = []() {
      auto all_hads = HADsState::HADs{
          "HAD #0", "HAD #1", "HAD #2", "HAD #3", "HAD #4", "HAD #5",
          "HAD #6", "HAD #7", "HAD #8", "HAD #9", "HAD #10", "HAD #11",
          "HAD #12", "HAD #13", "HAD #14", "HAD #15", "HAD #16", "HAD #17",
          "HAD #18", "HAD #19", "HAD #20", "HAD #21", "HAD #22", "HAD #23"};
      return std::make_shared<HADsState>(all_hads);
    };

    try {
      auto hads = hads_from_environment(m_parent_environment_ref);
      // TODO: Use hads to expose HAD options to the user

      this->add_option('0', {"HAD:s", HADs_factory});

    } catch (std::exception const &e) {
      spdlog::error("Error initializing FiscalYearState: {}", e.what());
      this->add_option('?', {"HAD:s", HADs_factory});
    }
  }
} // namespace first
