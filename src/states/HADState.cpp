
#include "HADState.hpp"

namespace first {
// ----------------------------------
  HADState::HADState(HAD had) : m_had{had} ,StateImpl({}) {
    ux().clear();
    ux().push_back(to_string(had));
    this->add_option('0',{"HAD -> SIE",SIE_factory});
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