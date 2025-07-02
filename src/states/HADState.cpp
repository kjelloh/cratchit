
#include "HADState.hpp"
#include "DeleteItemState.hpp"

namespace first {
// ----------------------------------
  HADState::HADState(HAD had) : m_had{had} ,StateImpl({}) {

    StateFactory sie_factory = []() {
      auto SIE_ux = StateImpl::UX{"HAD to SIE UX goes here"};
      return std::make_shared<StateImpl>(SIE_ux);
    };

    ux().clear();
    ux().push_back(to_string(had));
    this->add_cmd_option('0',{"HAD -> SIE", cmd_from_state_factory(sie_factory)});

    this->add_cmd_option('d', {"Delete",[had = this->m_had]() -> std::optional<Msg> {
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

} // namespace first