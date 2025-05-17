#pragma once

#include "StateImpl.hpp"

namespace first {
  // ----------------------------------
  struct VATReturnsState : public StateImpl {
    VATReturnsState(StateImpl::UX ux);
  }; // struct VATReturnsState
}