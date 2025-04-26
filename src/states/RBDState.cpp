
#include "RBDState.hpp"

namespace first {
// ----------------------------------
  RBDState::RBDState(RBD rbd) : m_rbd{rbd} ,StateImpl({}) {
    ux().clear();
    ux().push_back(rbd);
    this->add_option('0',{"RBD -> SIE",SIE_factory});
  }

} // namespace first