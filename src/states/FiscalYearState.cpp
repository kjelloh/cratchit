#include "FiscalYearState.hpp"
#include "HADsState.hpp"
#include "fiscal/amount/HADFramework.hpp"
#include "cargo/HADsCargo.hpp"
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

  std::pair<std::optional<State>, Cmd> FiscalYearState::update(Msg const &msg) {
    spdlog::info("FiscalYearState::update - BEGIN");
    std::optional<State> muated_state{};
    Cmd cmd{Nop};
    if (auto popped_state_msg_ptr = std::dynamic_pointer_cast<PoppedStateCargoMsg>(msg); popped_state_msg_ptr != nullptr) {
      spdlog::info("FiscalYearState::update - PoppedStateCargoMsg ok");
      if (auto hads_cargo_ptr = dynamic_cast<CargoImpl<HeadingAmountDateTransEntries>*>(popped_state_msg_ptr->m_cargo.get()); hads_cargo_ptr != nullptr) {
        spdlog::info("FiscalYearState::update - Received HADsCargo ok");
      }
    }
    spdlog::info("FiscalYearState::update - END");
    return {muated_state, cmd};
  }

} // namespace first
