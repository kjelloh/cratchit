#include "FiscalPeriodState.hpp"
#include "HADsState.hpp"
#include "fiscal/amount/HADFramework.hpp"
#include "cargo/HADsCargo.hpp"
#include "VATReturnsState.hpp"
#include "environment.hpp"
#include <spdlog/spdlog.h>
#include <ranges>

namespace first {

  FiscalPeriodState::FiscalPeriodState(
     StateImpl::UX ux
    ,FiscalPeriod fiscal_period
    ,HeadingAmountDateTransEntries period_hads)
      :  StateImpl{ux}
        ,m_fiscal_period{fiscal_period}
        ,m_period_hads{period_hads} {
    try {
      // Options
      this->add_cmd_option('h', HADsState::cmd_option_from(this->m_period_hads,this->m_fiscal_period));
      this->add_cmd_option('v', VATReturnsState::cmd_option_from());

    } catch (std::exception const &e) {
      spdlog::error("Error initializing FiscalPeriodState: {}", e.what());
    }
  }

  FiscalPeriodState::FiscalPeriodState(
     StateImpl::UX ux
    ,FiscalPeriod fiscal_period
    ,Environment const &parent_environment_ref)
      : FiscalPeriodState(ux,fiscal_period,to_period_hads(fiscal_period,parent_environment_ref)) {}

  StateUpdateResult FiscalPeriodState::update(Msg const& msg) const {
    // Not used for now ( See apply for update on child state Cargo)
    spdlog::info("FiscalPeriodState::update - didn't handle message");
    return {std::nullopt, Cmd{}}; // Didn't handle - let base dispatch use fallback
  }

  StateUpdateResult FiscalPeriodState::apply(cargo::HADsCargo const& cargo) const {
    std::optional<State> mutated_state{};
    Cmd cmd{Nop};
    spdlog::info("FiscalPeriodState::apply(cargo::HADsCargo)");
    if (cargo.m_payload != this->m_period_hads) {
      // Changes has been made
      spdlog::info("FiscalPeriodState::apply(cargo::HADsCargo) - HADs has changed. payload size: {}",
                   cargo.m_payload.size());
      mutated_state = to_cloned(*this, UX{}, this->m_fiscal_period, cargo.m_payload);
    }
    return {mutated_state, cmd};
  }

  Cargo FiscalPeriodState::get_cargo() const {
    EnvironmentPeriodSlice result{{},m_fiscal_period};
    // Represent current HADs into the environment slice
    result.environment()["HeadingAmountDateTransEntry"]; // Parent state 'diff' needs key to work also for empty slice
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
  StateImpl::CmdOption FiscalPeriodState::cmd_option_from(FiscalPeriod fiscal_period,Environment const& parent_environment_ref) {
    return {"Fiscal Period: " + fiscal_period.to_string(), cmd_from_state_factory(factory_from(fiscal_period, parent_environment_ref))};
  }
  StateImpl::CmdOption FiscalPeriodState::cmd_option_from(FiscalYear fiscal_year,Environment const& parent_environment_ref) {
    return {std::format("Fiscal Year: {}", fiscal_year.to_string()), cmd_from_state_factory(factory_from(fiscal_year.period(), parent_environment_ref))};
  }
  StateImpl::CmdOption FiscalPeriodState::cmd_option_from(FiscalQuarter fiscal_quarter,Environment const& parent_environment_ref) {
    return {std::format("Fiscal Quarter: {}", fiscal_quarter.to_string()), cmd_from_state_factory(factory_from(fiscal_quarter.period(), parent_environment_ref))};
  }

} // namespace first
