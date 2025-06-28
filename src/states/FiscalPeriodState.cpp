#include "FiscalPeriodState.hpp"
#include "HADsState.hpp"
#include "fiscal/amount/HADFramework.hpp"
#include "cargo/HADsCargo.hpp"
#include "VATReturnsState.hpp"
#include "environment.hpp"
#include <spdlog/spdlog.h>
#include <ranges>

namespace first {

  FiscalPeriodState::FiscalPeriodState(StateImpl::UX ux, FiscalPeriod fiscal_period, Environment const &parent_environment_ref)
      :  StateImpl{ux}
        ,m_fiscal_period{fiscal_period}
        ,m_period_hads{to_period_hads(fiscal_period,parent_environment_ref)} {

    try {
      // Options
      this->add_option('h', HADsState::option_from(this->m_period_hads,this->m_fiscal_period));
      this->add_option('v', VATReturnsState::option_from());

    } catch (std::exception const &e) {
      spdlog::error("Error initializing FiscalPeriodState: {}", e.what());
    }
  }

  std::pair<std::optional<State>, Cmd> FiscalPeriodState::update(Msg const &msg) {
    // Not used for now ( See apply for update on child state Cargo)
    spdlog::info("FiscalPeriodState::update - BEGIN");
    spdlog::info("FiscalPeriodState::update - No Operation");
    spdlog::info("FiscalPeriodState::update - END");
    return {std::nullopt, Nop};
  }

  std::pair<std::optional<State>, Cmd> FiscalPeriodState::apply(cargo::HADsCargo const& cargo) const {
    std::optional<State> mutated_state{};
    Cmd cmd{Nop};
    spdlog::info("FiscalPeriodState::apply(cargo::HADsCargo)");
    if (cargo.m_payload != this->m_period_hads) {
      // Changes has been made
      spdlog::info("FiscalPeriodState::apply(cargo::HADsCargo) - HADs has changed. payload size: {}",
                   cargo.m_payload.size());
      auto new_state = std::make_shared<FiscalPeriodState>(*this);
      new_state->m_period_hads = cargo.m_payload; // mutate HADs
      mutated_state = new_state;
    }
    return {mutated_state, cmd};
  }

  Cargo FiscalPeriodState::get_cargo() const {
    EnvironmentPeriodSlice result{{},m_fiscal_period};
    // Represent current HADs into the environment slice
    for (auto const& [index, env_val] : indexed_env_entries_from(this->m_period_hads)) {
        result.environment()["HeadingAmountDateTransEntry"].push_back({index, env_val});
    }
    return cargo::to_cargo(result);
  }

  StateFactory FiscalPeriodState::factory_from(FiscalPeriod fiscal_period,Environment const& parent_environment_ref) {
    return [fiscal_period, &parent_environment_ref]() {
      StateImpl::UX ux{"Fiscal Period: " + fiscal_period.to_string()};
      return std::make_shared<FiscalPeriodState>(ux, fiscal_period, parent_environment_ref);
    };
  }
  StateImpl::Option FiscalPeriodState::option_from(FiscalYear fiscal_year,Environment const& parent_environment_ref) {
    return {std::format("Fiscal Year: {}",fiscal_year.to_string()), factory_from(fiscal_year.period(), parent_environment_ref)};
  }
  StateImpl::Option FiscalPeriodState::option_from(FiscalQuarter fiscal_quarter,Environment const& parent_environment_ref) {
    return {std::format("Fiscal Quarter: {}",fiscal_quarter.to_string()), factory_from(fiscal_quarter.period(), parent_environment_ref)};
  }

} // namespace first
