#pragma once

#include "StateImpl.hpp"

namespace first {
  // ----------------------------------
  struct VATReturnsState : public StateImpl {
    VATReturnsState();

    virtual StateImpl::UpdateOptions create_update_options() const override;
    virtual std::string caption() const override;

    // static StateFactory factory_from();
    // static StateImpl::CmdOption cmd_option_from();

  }; // struct VATReturnsState
}