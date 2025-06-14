#include "FiscalYearState.hpp"
#include "HADsState.hpp"
#include "fiscal/amount/HADFramework.hpp"
#include <spdlog/spdlog.h>

namespace first {
  FiscalYearState::FiscalYearState(
      StateImpl::UX ux, FiscalPeriod fiscal_year, Environment const &parent_environment_ref)
      : StateImpl{ux}, m_fiscal_year{fiscal_year}, m_parent_environment_ref{parent_environment_ref} {

    try {
      StateFactory HADs_factory = [&env = this->m_parent_environment_ref]() {
        auto all_hads = hads_from_environment(env);
        return std::make_shared<HADsState>(all_hads);
      };

      this->add_option('0', {"HAD:s", HADs_factory});

    } catch (std::exception const &e) {
      spdlog::error("Error initializing FiscalYearState: {}", e.what());
    }
  }
} // namespace first
