
#include "HADState.hpp"
#include "DeleteItemState.hpp"
#include "msgs/msg.hpp"
#include "spdlog/spdlog.h"


namespace first {
// ----------------------------------
  HADState::HADState(HAD had) : m_edited_had{had, cargo::ItemMutation::UNCHANGED}, StateImpl({}) {

    // StateFactory sie_factory = []() {
    //   auto SIE_ux = StateImpl::UX{"HAD to SIE UX goes here"};
    //   return std::make_shared<StateImpl>(SIE_ux);
    // };

    // update_ux();

    // this->add_cmd_option('0',{"HAD -> SIE", cmd_from_state_factory(sie_factory)});

    // this->add_cmd_option('d', {"Delete",[had = this->m_edited_had.item]() -> std::optional<Msg> {
    //     State new_state = DeleteItemState<HAD>::factory_from(had)();
    //     auto msg = std::make_shared<PushStateMsg>(new_state);
    //     return msg;
    // }});
    // this->add_update_option('o', {"Ok", [this]() -> StateUpdateResult {
    //   spdlog::info("HADState 'o' lambda: capturing m_edited_had: {}", to_string(this->m_edited_had.item));
    //   using EditedHADMsg = CargoMsgT<cargo::EditedItem<HAD>>;
    //   return {std::nullopt, [payload = this->m_edited_had]() -> std::optional<Msg> {
    //     spdlog::info("HADState 'o' lambda execution: payload: {}", to_string(payload.item));
    //     return std::make_shared<PopStateMsg>(
    //       std::make_shared<EditedHADMsg>(payload)
    //     );
    //   }};
    // }});    
  }

  StateFactory HADState::factory_from(HADState::HAD const& had) {
    return [had]() {
      auto HAD_ux = StateImpl::UX{
        "HAD UX goes here"
      };
      return std::make_shared<HADState>(had);
    };
  }
  
  // StateImpl::CmdOption HADState::cmd_option_from(HADState::HAD const& had) {
  //   return {to_string(had), cmd_from_state_factory(factory_from(had))};
  // }

  // StateUpdateResult HADState::apply(cargo::EditedItemCargo<HAD> const& cargo) const {
  //   auto new_state = std::make_shared<HADState>(*this);
  //   new_state->m_edited_had = cargo.m_payload;  // Direct assignment!
  //   new_state->update_ux();
  //   return {new_state, Nop};
  // }
  StateUpdateResult HADState::update(Msg const& msg) const {
    using EditedHADMsg = CargoMsgT<cargo::EditedItem<HAD>>;
    if (auto pimpl = std::dynamic_pointer_cast<EditedHADMsg>(msg); pimpl != nullptr) {
      std::shared_ptr<HADState> new_state = to_cloned(*this, pimpl->payload.item);
      new_state->m_edited_had = pimpl->payload;
      spdlog::info("HADState::update - Received EditedHADMsg with payload {}, new_state->m_edited_had {} "
        ,to_string(pimpl->payload.item)
        ,to_string(new_state->m_edited_had.item));
      // UX now lazy-generated via create_ux() - no need to call update_ux()
      return {new_state, Nop};
    }
    return {};
  }

  // Cargo HADState::get_cargo() const {
  //   return cargo::to_cargo(m_edited_had);
  // }
  std::optional<Msg> HADState::get_on_destruct_msg() const {
    using EditedHADMsg = CargoMsgT<cargo::EditedItem<HAD>>;
    return std::make_shared<EditedHADMsg>(m_edited_had);
  }

  // void HADState::update_ux() {
  //   ux().clear();
    
  //   std::string status_prefix;
  //   switch (m_edited_had.mutation) {
  //     case cargo::ItemMutation::UNCHANGED:
  //       status_prefix = "";
  //       break;
  //     case cargo::ItemMutation::MODIFIED:
  //       status_prefix = "[MODIFIED] ";
  //       break;
  //     case cargo::ItemMutation::DELETED:
  //       status_prefix = "[MARKED FOR DELETION] ";
  //       break;
  //   }
    
  //   ux().push_back(status_prefix + to_string(m_edited_had.item));
  // }

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
      auto SIE_ux = StateImpl::UX{"HAD to SIE UX goes here"}; // Placeholder state
      return std::make_shared<StateImpl>(SIE_ux);
    };

    result.add('0',{"HAD -> SIE",[sie_factory]() -> StateUpdateResult {
      return {std::nullopt,cmd_from_state_factory(sie_factory)};
    }});

    result.add('d',{"Delete",[had = this->m_edited_had.item]() -> StateUpdateResult {
      return {std::nullopt,[had]() -> std::optional<Msg> {
        State new_state = DeleteItemState<HAD>::factory_from(had)();
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