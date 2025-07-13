#pragma once

#include "StateImpl.hpp"

namespace first {
  // ----------------------------------
  struct VATReturnsState : public StateImpl {
    VATReturnsState(StateImpl::UX ux);

    virtual StateImpl::UpdateOptions create_update_options() const override;

    static StateFactory factory_from();
    static StateImpl::CmdOption cmd_option_from();

  }; // struct VATReturnsState
}