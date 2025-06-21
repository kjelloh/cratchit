#include "FiscalYearState.hpp"
#include "HADsState.hpp"
#include "fiscal/amount/HADFramework.hpp"
#include "cargo/HADsCargo.hpp"
#include <spdlog/spdlog.h>
#include <ranges>

namespace first {

  auto ev_to_maybe_had = [](EnvironmentValue const &ev) -> OptionalHeadingAmountDateTransEntry { return to_had(ev); };

  auto in_period = [](HeadingAmountDateTransEntry const &had, FiscalPeriod const &period) -> bool { return period.contains(had.date); };

  auto id_ev_pair_to_ev = [](EnvironmentIdValuePair const& id_ev_pair) {return id_ev_pair.second;};

  auto to_period_hads(FiscalPeriod period, const Environment &env) -> HeadingAmountDateTransEntries {

    static constexpr auto section = "HeadingAmountDateTransEntry";

    if (!env.contains(section)) {
      return {}; // No entries of this type
    }

    auto const &id_ev_pairs = env.at(section);

    return id_ev_pairs
           | std::views::transform(id_ev_pair_to_ev)
           | std::views::transform(ev_to_maybe_had)
           | std::views::filter([](auto const &maybe_had) { return maybe_had.has_value(); })
           | std::views::transform([](auto const &maybe_had) { return *maybe_had; })
           | std::views::filter([&](auto const &had) { return in_period(had, period); })
           | std::ranges::to<std::vector>();
  }

  FiscalYearState::FiscalYearState(
      StateImpl::UX ux, FiscalPeriod fiscal_period, Environment const &parent_environment_ref)
      :  StateImpl{ux}
        ,m_fiscal_period{fiscal_period}
        ,m_period_hads{to_period_hads(fiscal_period,parent_environment_ref)} {

    try {
      StateFactory HADs_factory = [&period_hads = this->m_period_hads]() {
        return std::make_shared<HADsState>(period_hads);
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
