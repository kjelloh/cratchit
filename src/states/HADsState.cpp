#include "HADsState.hpp"
#include "HADState.hpp"
#include "tokenize.hpp"
#include <spdlog/spdlog.h>
#include <algorithm> // std::ranges::find,

namespace first {
  // ----------------------------------
  HADsState::HADsState(HADs all_hads,FiscalPeriod fiscal_period,Mod10View mod10_view)
    :  m_all_hads{all_hads}
      ,m_fiscal_period{fiscal_period}
      ,m_mod10_view{mod10_view}
      ,StateImpl() {

    spdlog::info("HADsState::HADsState(HADs all_hads:size={},Mod10View mod10_view)",m_all_hads.size());

  }

  // ----------------------------------

  HADsState::HADsState(HADs all_hads,FiscalPeriod fiscal_period) 
    : HADsState(all_hads,fiscal_period,Mod10View(all_hads)) {}

  // ----------------------------------

  StateImpl::UX HADsState::create_ux() const {
    // Create view UX
    UX result{};
    result.push_back(std::format("{}", m_fiscal_period.to_string()));
    for (size_t i=m_mod10_view.m_range.first;i<m_mod10_view.m_range.second;++i) {
      auto entry = std::to_string(i);
      entry += ". ";
      entry += to_string(m_all_hads[i]);
      result.push_back(entry);
    }
    return result;
  }

  // ----------------------------------

  StateUpdateResult HADsState::update(Msg const& msg) const {
    using EditedHADMsg = CargoMsgT<cargo::EditedItem<HAD>>;
    using HADsMsg = CargoMsgT<HADsState::HADs>;
    if (auto entry_msg_ptr = std::dynamic_pointer_cast<UserEntryMsg>(msg);entry_msg_ptr != nullptr) {
      spdlog::info("HADsState::update - handling UserEntryMsg");
      std::string command(entry_msg_ptr->m_entry);
      auto tokens = tokenize::splits(command,tokenize::SplitOn::TextAmountAndDate);
      if (auto had = to_had(tokens)) {
        auto mutated_hads = this->m_all_hads;
        mutated_hads.push_back(*had); // TODO: Check that new HAD is in period
        auto mutated_state = make_state<HADsState>(mutated_hads, m_fiscal_period);
        return {mutated_state, Cmd{}};
      }
      else {
        spdlog::info("HADsState::update - not a had");
      }
    }
    else if (auto pimpl = std::dynamic_pointer_cast<HADsMsg>(msg); pimpl != nullptr) {
      // We have got cargo from a subrange HADsState - reporting its subrange HADs
      spdlog::info("HADsState::update - handling HADsMsg");
      if (this->m_all_hads != pimpl->payload) {
        spdlog::info("HADsState::update - HADs has changed");
        auto mutated_hads = pimpl->payload; // Use payload as the new truth
        auto mutated_state = make_state<HADsState>(mutated_hads,this->m_fiscal_period);
        return {mutated_state, Cmd{}};
      }
    }
    else if (auto pimpl = std::dynamic_pointer_cast<EditedHADMsg>(msg); pimpl != nullptr) {
      spdlog::info("HADsState::update - handling EditedHADMsg");
      auto mutated_hads = this->m_all_hads;
      MaybeState maybe_state{}; // default 'none'
      // Remove the HAD from the collection in the mutated state
      if (auto iter = std::ranges::find(mutated_hads,pimpl->payload.item);iter != mutated_hads.end()) {
        mutated_hads.erase(iter);
        spdlog::info("HADsState::update - HAD {} DELETED ok",to_string(*iter));
        maybe_state = make_state<HADsState>(mutated_hads, this->m_fiscal_period);
      }
      else {
        spdlog::error("DESIGN_INSUFFICIENCY: HADsState::update() for EditedHADMsg failed to find HAD {} to delete",to_string(pimpl->payload.item));
      }      
      return {maybe_state, Cmd{}};
    }
    return {};
  }


  StateImpl::UpdateOptions HADsState::create_update_options() const {
    StateImpl::UpdateOptions result{};
    
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

    auto subranges = m_mod10_view.subranges();
    for (size_t i=0;i<subranges.size();++i) {
      auto const subrange = subranges[i];
      auto const& [begin,end] = subrange;
      char key = static_cast<char>('0'+i);
      
      if (end-begin==1) {
        // Single HAD in range option - convert to update option
        auto caption = to_string(m_all_hads[begin]);
        result.add(key, {caption, [had = m_all_hads[begin]]() -> StateUpdateResult {
          return {std::nullopt, [had]() -> std::optional<Msg> {
            State new_state = make_state<HADState>(had);
            return std::make_shared<PushStateMsg>(new_state);
          }};
        }});
      }
      else {
        // Subrange option - convert to update option
        auto caption = std::to_string(subrange.first) + " .. " + std::to_string(subrange.second-1);
        result.add(key, {caption, [all_hads = m_all_hads, fiscal_period = m_fiscal_period, subrange]() -> StateUpdateResult {
          return {std::nullopt, [all_hads, fiscal_period, subrange]() -> std::optional<Msg> {
            auto factory = HADs_subrange_factory(all_hads, fiscal_period, subrange);
            State new_state = factory();
            return std::make_shared<PushStateMsg>(new_state);
          }};
        }});
      }
    }
    
    // Add '-' key option last - back with HADs as payload
    result.add('-', {"Back", [this]() -> StateUpdateResult {
      using HADsMsg = CargoMsgT<HADsState::HADs>;
      return {std::nullopt, [all_hads = this->m_all_hads]() -> std::optional<Msg> {
        return std::make_shared<PopStateMsg>(
          std::make_shared<HADsMsg>(all_hads)
        );
      }};
    }});
    
    return result;
  }

} // namespace first