#pragma once

#include "StateImpl.hpp"
#include "fiscal/amount/AccountStatement.hpp"
#include "cargo/CargoBase.hpp"

namespace first {
  class AccountStatementState : public StateImpl {
  public:
    using ExpectedAccountStatement = ::ExpectedAccountStatement;    
    AccountStatementState(ExpectedAccountStatement expected_account_statement);
  private:
    ExpectedAccountStatement m_expected_account_statement;
    virtual std::string caption() const override;
    virtual StateImpl::UpdateOptions create_update_options() const override;
    virtual UX create_ux() const override;
  };
} // namespace first
