#include "HADsState.hpp"
#include "HADState.hpp"
#include "tokenize.hpp"
#include "logger/log.hpp"
#include <algorithm> // std::ranges::find,

namespace first {
  // ----------------------------------
  // HADsState::HADsState(HADs all_hads,FiscalPeriod fiscal_period,Mod10View mod10_view)
  //   :  StateImpl()
  //     ,m_all_hads{all_hads}
  //     ,m_fiscal_period{fiscal_period}
  //     ,m_hads_slice{fiscal_period,all_hads}
  //     ,m_mod10_view{mod10_view} {

  //   logger::development_trace("HADsState::HADsState(HADs all_hads:size={},Mod10View mod10_view)",m_all_hads.size());

  // }

  // // ----------------------------------

  // HADsState::HADsState(HADs all_hads,FiscalPeriod fiscal_period) 
  //   : HADsState(all_hads,fiscal_period,Mod10View(all_hads)) {}

  // ----------------------------------

  // 'newer'
  HADsState::HADsState(HADsSlice const& hads_slice,Mod10View mod10_view)
    :  StateImpl()
      // ,m_all_hads{hads_slice.content()}
      // ,m_fiscal_period{hads_slice.period()}
      ,m_hads_slice{hads_slice}
      ,m_mod10_view{mod10_view} {

    logger::development_trace("HADsState::HADsState(HADsSlice const& hads_slice::size={},Mod10View mod10_view)",hads_slice.content().size());

  }

  HADsState::HADsState(HADsSlice const& hads_slice)
    : HADsState(hads_slice,Mod10View(hads_slice.content())) {}
  // ----------------------------------


  StateImpl::UX HADsState::create_ux() const {
    // Create view UX
    UX result{};
    result.push_back(std::format("{}", m_hads_slice.period().to_string()));
    for (size_t i : m_mod10_view) {
      auto entry = std::to_string(i);
      entry += ". ";
      entry += to_string(m_hads_slice.content()[i]);
      result.push_back(entry);
    }
    return result;
  }

  // ----------------------------------

  StateUpdateResult HADsState::update(Msg const& msg) const {
    using EditedHADMsg = CargoMsgT<cargo::EditedItem<HAD>>;
    using HADsMsg = CargoMsgT<HADsState::HADs>;
    if (auto entry_msg_ptr = std::dynamic_pointer_cast<UserEntryMsg>(msg);entry_msg_ptr != nullptr) {
      logger::development_trace("HADsState::update - handling UserEntryMsg");
      std::string command(entry_msg_ptr->m_entry);
      auto tokens = tokenize::splits(command,tokenize::SplitOn::TextAmountAndDate);
      if (auto had = to_had(tokens)) {
        if (this->m_hads_slice.period().contains(had->date)) {
          auto mutated_hads = this->m_hads_slice.content();
          mutated_hads.push_back(had.value()); 
          // Sort by date for consistent ordering
          std::sort(mutated_hads.begin(), mutated_hads.end(), date_ordering);
          auto mutated_state = make_state<HADsState>(HADsSlice{this->m_hads_slice.period(),mutated_hads});
          return {mutated_state, Cmd{}};
        }
        else {
          // TODO: Design a way to show input error to use in this state
          logger::design_insufficiency("HADsState::update: No error message for user input of HAD outside period");
        }
      }
      else {
        logger::design_insufficiency("HADsState::update: No error message for user input 'not a HAD'");
      }
    }
    else if (auto pimpl = std::dynamic_pointer_cast<HADsMsg>(msg); pimpl != nullptr) {
      // We have got cargo from a subrange HADsState - reporting its subrange HADs
      logger::development_trace("HADsState::update - handling HADsMsg");
      if (this->m_hads_slice.content() != pimpl->payload) {
        logger::development_trace("HADsState::update - HADs has changed");
        auto mutated_hads = pimpl->payload; // Use payload as the new truth
        // Sort by date for consistent ordering
        std::sort(mutated_hads.begin(), mutated_hads.end(), date_ordering);
        auto mutated_state = make_state<HADsState>(HADsSlice{this->m_hads_slice.period(),mutated_hads});
        return {mutated_state, Cmd{}};
      }
    }
    else if (auto pimpl = std::dynamic_pointer_cast<EditedHADMsg>(msg); pimpl != nullptr) {
      logger::development_trace("HADsState::update - handling EditedHADMsg");
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
        auto caption = to_string(m_hads_slice.content()[*index]);
        result.add(digit, {caption, [had = m_hads_slice.content()[*index]]() -> StateUpdateResult {
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
          result.add(digit, {caption, [all_hads = m_hads_slice.content(), fiscal_period = m_hads_slice.period(), drilled_view]() -> StateUpdateResult {
            return {std::nullopt, [all_hads, fiscal_period, drilled_view]() -> std::optional<Msg> {
              State new_state = make_state<HADsState>(HADsSlice{fiscal_period,all_hads}, drilled_view);
              return std::make_shared<PushStateMsg>(new_state);
            }};
          }});
        } catch (const std::exception& e) {
          logger::design_insufficiency("HADsState: Error creating option for digit '{}': {}", digit, e.what());
        }
      }
    }
    
    // Add '-' key option last - back with HADs as payload
    result.add('-', {"Back", [this]() -> StateUpdateResult {
      using HADsMsg = CargoMsgT<HADsState::HADs>;
      return {std::nullopt, [all_hads = this->m_hads_slice.content()]() -> std::optional<Msg> {
        return std::make_shared<PopStateMsg>(
          std::make_shared<HADsMsg>(all_hads)
        );
      }};
    }});
    
    return result;
  }

  StateUpdateResult HADsState::apply(cargo::EditedItem<HAD> const& edited_had) const {
    auto mutated_hads = this->m_hads_slice.content();
    MaybeState maybe_state{}; // default 'none'
    
    switch (edited_had.mutation) {
      case cargo::ItemMutation::DELETED:
        // Remove the HAD from the collection
        if (auto iter = std::ranges::find(mutated_hads, edited_had.item); iter != mutated_hads.end()) {
          mutated_hads.erase(iter);
          logger::development_trace("HADsState::apply - HAD {} DELETED ok", to_string(*iter));
          maybe_state = make_state<HADsState>(HADsSlice{this->m_hads_slice.period(),mutated_hads});
        }
        else {
          logger::design_insufficiency("HADsState::apply - failed to find HAD {} to delete", to_string(edited_had.item));
        }
        break;
        
      case cargo::ItemMutation::MODIFIED:
        // Future: replace existing HAD with modified version
        logger::development_trace("HADsState::apply - HAD {} marked as MODIFIED (not yet implemented)", to_string(edited_had.item));
        break;
        
      case cargo::ItemMutation::UNCHANGED:
        logger::development_trace("HADsState::apply - HAD {} returned unchanged", to_string(edited_had.item));
        // No state change needed
        break;
    }
    
    return {maybe_state, Cmd{}};
  }

} // namespace first