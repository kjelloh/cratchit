#include "HADsState.hpp"
#include "HADState.hpp"
#include "tokenize.hpp"
#include "log/log.hpp"
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
    for (size_t i : m_mod10_view) {
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
        // Sort by date for consistent ordering
        std::sort(mutated_hads.begin(), mutated_hads.end(), date_ordering);
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
        // Sort by date for consistent ordering
        std::sort(mutated_hads.begin(), mutated_hads.end(), date_ordering);
        auto mutated_state = make_state<HADsState>(mutated_hads,this->m_fiscal_period);
        return {mutated_state, Cmd{}};
      }
    }
    else if (auto pimpl = std::dynamic_pointer_cast<EditedHADMsg>(msg); pimpl != nullptr) {
      spdlog::info("HADsState::update - handling EditedHADMsg");
      return apply(pimpl->payload);
    }
    return {};
  }


  StateImpl::UpdateOptions HADsState::create_update_options() const {
    StateImpl::UpdateOptions result{};
    
    // Use enhanced API for cleaner digit-based option creation
    for (char digit = '0'; digit <= '9'; ++digit) {
      if (!m_mod10_view.is_valid_digit(digit)) {
        break; // No more valid digits
      }
      
      // Try to get direct index for single items
      if (auto index = m_mod10_view.direct_index(digit); index.has_value()) {
        // Single HAD - direct navigation to HADState
        auto caption = to_string(m_all_hads[*index]);
        result.add(digit, {caption, [had = m_all_hads[*index]]() -> StateUpdateResult {
          return {std::nullopt, [had]() -> std::optional<Msg> {
            State new_state = make_state<HADState>(had);
            return std::make_shared<PushStateMsg>(new_state);
          }};
        }});
      } else {
        // Subrange - use drill_down for navigation
        try {
          auto drilled_view = m_mod10_view.drill_down(digit);
          auto caption = std::format("{} .. {}", drilled_view.first(), drilled_view.second() - 1);
          result.add(digit, {caption, [all_hads = m_all_hads, fiscal_period = m_fiscal_period, drilled_view]() -> StateUpdateResult {
            return {std::nullopt, [all_hads, fiscal_period, drilled_view]() -> std::optional<Msg> {
              State new_state = make_state<HADsState>(all_hads, fiscal_period, drilled_view);
              return std::make_shared<PushStateMsg>(new_state);
            }};
          }});
        } catch (const std::exception& e) {
          spdlog::error("HADsState: Error creating option for digit '{}': {}", digit, e.what());
        }
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

  StateUpdateResult HADsState::apply(cargo::EditedItem<HAD> const& edited_had) const {
    auto mutated_hads = this->m_all_hads;
    MaybeState maybe_state{}; // default 'none'
    
    switch (edited_had.mutation) {
      case cargo::ItemMutation::DELETED:
        // Remove the HAD from the collection
        if (auto iter = std::ranges::find(mutated_hads, edited_had.item); iter != mutated_hads.end()) {
          mutated_hads.erase(iter);
          spdlog::info("HADsState::apply - HAD {} DELETED ok", to_string(*iter));
          maybe_state = make_state<HADsState>(mutated_hads, this->m_fiscal_period);
        }
        else {
          spdlog::error("HADsState::apply - failed to find HAD {} to delete", to_string(edited_had.item));
        }
        break;
        
      case cargo::ItemMutation::MODIFIED:
        // Future: replace existing HAD with modified version
        spdlog::info("HADsState::apply - HAD {} marked as MODIFIED (not yet implemented)", to_string(edited_had.item));
        break;
        
      case cargo::ItemMutation::UNCHANGED:
        spdlog::info("HADsState::apply - HAD {} returned unchanged", to_string(edited_had.item));
        // No state change needed
        break;
    }
    
    return {maybe_state, Cmd{}};
  }

} // namespace first