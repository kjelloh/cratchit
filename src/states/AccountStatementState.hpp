#pragma once

#include "StateImpl.hpp"
#include "fiscal/amount/AccountStatement.hpp"
#include "cargo/CargoBase.hpp"
#include "../PeriodConstrainedContent.hpp"
#include "functional/maybe.hpp"

namespace first {
  class AccountStatementState : public StateImpl {
  public:
    using AnnotatedAccountStatement = AnnotatedMaybe<AccountStatement>;
    using PeriodPairedAnnotatedAccountStatement = PeriodPairedT<AnnotatedAccountStatement>;
    // AccountStatementState(AnnotatedAccountStatement annotated_account_statement);
    AccountStatementState(PeriodPairedAnnotatedAccountStatement period_paired_annotated_account_statement);
  private:
    // AnnotatedAccountStatement m_annotated_account_statement;
    PeriodPairedAnnotatedAccountStatement m_period_paired_annotated_account_statement;
    virtual std::string caption() const override;
    virtual StateImpl::UpdateOptions create_update_options() const override;
    virtual UX create_ux() const override;
  };
} // namespace first
