#include "AccountStatementState.hpp"
#include "DeleteItemState.hpp"
#include "msgs/msg.hpp"
#include "logger/log.hpp"
#include <format>

namespace first {

  AccountStatementState::AccountStatementState(ExpectedAccountStatement expected_account_statement) 
    :  StateImpl()
      ,m_expected_account_statement{expected_account_statement} {}

  std::string AccountStatementState::caption() const {
    return "Account Statement";
  }

  StateImpl::UX AccountStatementState::create_ux() const {
    UX result{};
    result.push_back(this->caption());
    if (m_expected_account_statement) {
      result.push_back("TODO: Statement content table goes here");
    }
    else {
      result.push_back(std::format("Sorry - {}",m_expected_account_statement.error()));
    }
    return result;
  }

  StateImpl::UpdateOptions AccountStatementState::create_update_options() const {
    StateImpl::UpdateOptions result{};

    result.add('-', {"Back", []() -> StateUpdateResult {
      using StringMsg = CargoMsgT<std::string>;
      return {std::nullopt, []() -> std::optional<Msg> {
        return std::make_shared<PopStateMsg>(
          std::make_shared<StringMsg>("AccountStatementState")
        );
      }};
    }});

    return result;
  }

} // namespace first