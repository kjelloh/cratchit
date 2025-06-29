
#include "HADState.hpp"

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
} // namespace first