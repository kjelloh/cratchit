#include "HADsState.hpp"
#include "HADState.hpp"
#include "tokenize.hpp"
#include <spdlog/spdlog.h>

namespace first {
  // ----------------------------------
  HADsState::HADsState(HADs all_hads,FiscalPeriod fiscal_period,Mod10View mod10_view)
    :  m_all_hads{all_hads}
      ,m_fiscal_period{fiscal_period}
      ,m_mod10_view{mod10_view}
      ,StateImpl({}) {

    spdlog::info("HADsState::HADsState(HADs all_hads:size={},Mod10View mod10_view)",m_all_hads.size());

    // HAD subrange StateImpl factory
    // Enable 'drill down' modulo 10 into HADs
    struct HADs_subrange_factory {
      HADs_subrange_factory(HADsState::HADs all_hads, FiscalPeriod fiscal_period,Mod10View mod10_view)
          : m_all_hads{all_hads},m_fiscal_period{fiscal_period},m_mod10_view{mod10_view} {}

      HADsState::HADs m_all_hads{};
      FiscalPeriod m_fiscal_period;
      Mod10View m_mod10_view;

      auto operator()() {
        return std::make_shared<HADsState>(m_all_hads, m_fiscal_period,m_mod10_view);
      }

    };

    auto option_from = [this](Mod10View::Range const& subrange) -> StateImpl::Option {
      auto caption = std::to_string(subrange.first);
      caption += " .. ";
      caption += std::to_string(subrange.second-1);
      return {caption, HADs_subrange_factory(m_all_hads,m_fiscal_period,subrange)};
    };

    auto subranges = m_mod10_view.subranges();
    for (size_t i=0;i<subranges.size();++i) {
      auto const subrange = subranges[i];
      auto const& [begin,end] = subrange;
      if (end-begin==1) {
        // Single HAD in range option
        this->add_option(static_cast<char>('0'+i),HADState::option_from(m_all_hads[begin]));
      }
      else {
        this->add_option(static_cast<char>('0'+i),option_from(subrange));
      }
    }

    // Initiate view UX
    this->ux().push_back(std::format("{}", m_fiscal_period.to_string()));
    for (size_t i=m_mod10_view.m_range.first;i<m_mod10_view.m_range.second;++i) {
      auto entry = std::to_string(i);
      entry += ". ";
      entry += to_string(m_all_hads[i]);
      this->ux().push_back(entry);
    }

  }

  // ----------------------------------
  HADsState::HADsState(HADs all_hads,FiscalPeriod fiscal_period) 
    : HADsState(all_hads,fiscal_period,Mod10View(all_hads)) {}

  std::pair<std::optional<State>, Cmd> HADsState::update(Msg const &msg) {
      if (auto entry_msg_ptr = std::dynamic_pointer_cast<UserEntryMsg>(msg);entry_msg_ptr != nullptr) {
        spdlog::info("HADsState::update - handling UserEntryMsg");
        std::string command(entry_msg_ptr->m_entry);
        auto tokens = tokenize::splits(command,tokenize::SplitOn::TextAmountAndDate);
        if (auto had = to_had(tokens)) {
          auto mutated_hads = this->m_all_hads;
          mutated_hads.push_back(*had); // TODO: Check that new HAD is in period
          auto mutated_state = std::make_shared<HADsState>(mutated_hads,m_fiscal_period);
          return {mutated_state, Cmd{}};
        }
        else {
          spdlog::info("HADsState::update - not a had");
        }
      }
      
      // Didn't handle - fall through to base dispatch
      return {std::nullopt, Cmd{}};
  }

  Cargo HADsState::get_cargo() const {
    return cargo::to_cargo(this->m_all_hads);
  }

  StateFactory HADsState::factory_from(HADsState::HADs const& all_hads,FiscalPeriod fiscal_period) {
    // Called by parent state so all_hads will exist as long as this callable is avaibale (option in parent state)
    return [&all_hads,fiscal_period]() {
      auto ux = StateImpl::UX{"HADs UX goes here"};
      return std::make_shared<HADsState>(all_hads,fiscal_period);
    };
  }

  StateImpl::Option HADsState::option_from(HADs const& all_hads,FiscalPeriod fiscal_period) {
    return {std::format("HADs - count:{}",all_hads.size()), factory_from(all_hads,fiscal_period)};
  }
} // namespace first