#pragma once

#include "StateImpl.hpp"

namespace first {
  class AccountStatementsState : public StateImpl {
  public:
    AccountStatementsState();

    virtual StateImpl::UpdateOptions create_update_options() const override;
    virtual std::string caption() const override;

  }; // AccountStatementsState
}