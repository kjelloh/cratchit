
#include "HADState.hpp"
#include "DeleteItemState.hpp"
#include "msgs/msg.hpp"
#include "spdlog/spdlog.h"


namespace first {
// ----------------------------------
  HADState::HADState(HAD had) : m_edited_had{had, cargo::ItemMutation::UNCHANGED}, StateImpl() {

    
  }

  StateUpdateResult HADState::update(Msg const& msg) const {
    using EditedHADMsg = CargoMsgT<cargo::EditedItem<HAD>>;
    if (auto pimpl = std::dynamic_pointer_cast<EditedHADMsg>(msg); pimpl != nullptr) {
      auto new_state = make_state<HADState>(pimpl->payload.item);
      new_state->m_edited_had = pimpl->payload;
      spdlog::info("HADState::update - Received EditedHADMsg with payload {}, new_state->m_edited_had {} "
        ,to_string(pimpl->payload.item)
        ,to_string(new_state->m_edited_had.item));
      // UX now lazy-generated via create_ux() - no need to call update_ux()
      return {new_state, Nop};
    }
    return {};
  }


  StateImpl::UX HADState::create_ux() const {
    UX result{};
    std::string status_prefix;
    switch (m_edited_had.mutation) {
      case cargo::ItemMutation::UNCHANGED:
        status_prefix = "";
        break;
      case cargo::ItemMutation::MODIFIED:
        status_prefix = "[MODIFIED] ";
        break;
      case cargo::ItemMutation::DELETED:
        status_prefix = "[MARKED FOR DELETION] ";
        break;
    }
    
    result.push_back(status_prefix + to_string(m_edited_had.item));

    return result;
  }



  StateImpl::UpdateOptions HADState::create_update_options() const {
    StateImpl::UpdateOptions result{};

    StateFactory sie_factory = []() {
      return make_state<StateImpl>("HAD to SIE"); // Placeholder state
    };

    result.add('0',{"HAD -> SIE",[sie_factory]() -> StateUpdateResult {
      return {std::nullopt,[new_state = sie_factory()]() -> std::optional<Msg> {
        return std::make_shared<PushStateMsg>(new_state);
      }};
    }});

    result.add('d',{"Delete",[had = this->m_edited_had.item]() -> StateUpdateResult {
      return {std::nullopt,[had]() -> std::optional<Msg> {
        State new_state = make_state<DeleteItemState<HAD>>(had);
        auto msg = std::make_shared<PushStateMsg>(new_state);
        return msg;
      }};
    }});
    
    result.add('o', {"Ok", [this]() -> StateUpdateResult {
      spdlog::info("HADState 'o' lambda: capturing m_edited_had: {}", to_string(this->m_edited_had.item));
      using EditedHADMsg = CargoMsgT<cargo::EditedItem<HAD>>;
      return {std::nullopt, [payload = this->m_edited_had]() -> std::optional<Msg> {
        spdlog::info("HADState 'o' lambda execution: payload: {}", to_string(payload.item));
        return std::make_shared<PopStateMsg>(
          std::make_shared<EditedHADMsg>(payload)
        );
      }};
    }});
    
    // Add '-' key option last - back with current HAD state as cargo
    result.add('-', {"Back", [this]() -> StateUpdateResult {
      using EditedHADMsg = CargoMsgT<cargo::EditedItem<HAD>>;
      return {std::nullopt, [payload = this->m_edited_had]() -> std::optional<Msg> {
        return std::make_shared<PopStateMsg>(
          std::make_shared<EditedHADMsg>(payload)
        );
      }};
    }});
    
    // TODO: Refactor add_cmd_option in constructor to update options here
    return result;
  }

} // namespace first