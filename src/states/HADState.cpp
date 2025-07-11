
#include "HADState.hpp"
#include "DeleteItemState.hpp"
#include "msgs/msg.hpp"

namespace first {
// ----------------------------------
  HADState::HADState(HAD had) : m_edited_had{had, cargo::ItemMutation::UNCHANGED}, StateImpl({}) {

    StateFactory sie_factory = []() {
      auto SIE_ux = StateImpl::UX{"HAD to SIE UX goes here"};
      return std::make_shared<StateImpl>(SIE_ux);
    };

    update_ux();
    this->add_cmd_option('0',{"HAD -> SIE", cmd_from_state_factory(sie_factory)});

    this->add_cmd_option('d', {"Delete",[had = this->m_edited_had.item]() -> std::optional<Msg> {
        State new_state = DeleteItemState<HAD>::factory_from(had)();
        auto msg = std::make_shared<PushStateMsg>(new_state);
        return msg;
    }});
  }

  StateFactory HADState::factory_from(HADState::HAD const& had) {
    return [had]() {
      auto HAD_ux = StateImpl::UX{
        "HAD UX goes here"
      };
      return std::make_shared<HADState>(had);
    };
  }
  
  StateImpl::CmdOption HADState::cmd_option_from(HADState::HAD const& had) {
    return {to_string(had), cmd_from_state_factory(factory_from(had))};
  }

  StateUpdateResult HADState::apply(cargo::EditedItemCargo<HAD> const& cargo) const {
    auto new_state = std::make_shared<HADState>(*this);
    new_state->m_edited_had = cargo.m_payload;  // Direct assignment!
    new_state->update_ux();
    return {new_state, Nop};
  }

  Cargo HADState::get_cargo() const {
    return cargo::to_cargo(m_edited_had);
  }

  void HADState::update_ux() {
    ux().clear();
    
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
    
    ux().push_back(status_prefix + to_string(m_edited_had.item));
  }

} // namespace first