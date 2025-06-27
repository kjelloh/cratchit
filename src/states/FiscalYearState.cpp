#include "FiscalYearState.hpp"
#include "HADsState.hpp"
#include "fiscal/amount/HADFramework.hpp"
#include "cargo/HADsCargo.hpp"
#include "environment.hpp"
#include <spdlog/spdlog.h>
#include <ranges>

namespace first {

  FiscalYearState::FiscalYearState(StateImpl::UX ux, FiscalPeriod fiscal_period, Environment const &parent_environment_ref)
      :  StateImpl{ux}
        ,m_fiscal_period{fiscal_period}
        ,m_period_hads{to_period_hads(fiscal_period,parent_environment_ref)} {

    try {
      // Options
      this->add_option('0', HADsState::option_from(this->m_period_hads,this->m_fiscal_period));
    } catch (std::exception const &e) {
      spdlog::error("Error initializing FiscalYearState: {}", e.what());
    }
  }

  std::pair<std::optional<State>, Cmd> FiscalYearState::update(Msg const &msg) {
    // Not used for now ( See apply for update on child state Cargo)
    spdlog::info("FiscalYearState::update - BEGIN");
    spdlog::info("FiscalYearState::update - No Operation");
    spdlog::info("FiscalYearState::update - END");
    return {std::nullopt, Nop};
  }

  std::pair<std::optional<State>, Cmd> FiscalYearState::apply(cargo::HADsCargo const& cargo) const {
    std::optional<State> mutated_state{};
    Cmd cmd{Nop};
    spdlog::info("FiscalYearState::apply(cargo::HADsCargo)");
    if (cargo.m_payload != this->m_period_hads) {
      // Changes has been made
      spdlog::info("FiscalYearState::apply(cargo::HADsCargo) - HADs has changed. payload size: {}",
                   cargo.m_payload.size());
      auto new_state = std::make_shared<FiscalYearState>(*this);
      new_state->m_period_hads = cargo.m_payload; // mutate HADs
      mutated_state = new_state;
    }
    return {mutated_state, cmd};
  }

  Cargo FiscalYearState::get_cargo() const {
    EnvironmentPeriodSlice result{{},m_fiscal_period};
    // Represent current HADs into the environment slice
    for (auto const& [index, env_val] : indexed_env_entries_from(this->m_period_hads)) {
        result.environment()["HeadingAmountDateTransEntry"].push_back({index, env_val});
    }
    return cargo::to_cargo(result);
  }

} // namespace first
