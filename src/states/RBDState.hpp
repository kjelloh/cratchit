#pragma once

#include "StateImpl.hpp"

namespace first {
  // ----------------------------------
  struct RBDState : public StateImpl {
    StateFactory SIE_factory = []() {
      auto SIE_ux = StateImpl::UX{"RBD to SIE UX goes here"};
      return std::make_shared<StateImpl>(SIE_ux);
    };
    using RBD = std::string;
    RBD m_rbd;
    RBDState(RBD rbd);
  };
} // namespace first