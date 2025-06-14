
#include "HADState.hpp"

namespace first {
// ----------------------------------
  HADState::HADState(HAD had) : m_had{had} ,StateImpl({}) {
    ux().clear();
    ux().push_back(to_string(had));
    this->add_option('0',{"HAD -> SIE",SIE_factory});
  }

} // namespace first