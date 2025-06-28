#pragma once

#include "StateImpl.hpp"

namespace first {
  // ----------------------------------
  struct VATReturnsState : public StateImpl {
    VATReturnsState(StateImpl::UX ux);

    static StateFactory factory_from();
    static StateImpl::Option option_from();

  }; // struct VATReturnsState
}