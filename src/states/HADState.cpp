
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
    this->add_option('0',{"HAD -> SIE",sie_factory});

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
  StateImpl::Option HADState::option_from(HADState::HAD const& had) {
    return {to_string(had), factory_from(had)};
  }

  // TODO: Refactor key procesing into this method, step-by-step
  //       When done, move into update above (and remove this refactoring step)
  std::optional<Cmd> HADState::cmd_from_key(char key) const {
    return StateImpl::cmd_from_key(key);
  }

} // namespace first