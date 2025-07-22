#include "AccountStatementsState.hpp"
#include <format>

namespace first {
  
  AccountStatementsState::AccountStatementsState() : StateImpl{} {}

  std::string AccountStatementsState::caption() const {
    if (m_caption.has_value()) {
      return m_caption.value();
    }
    return "Account Statements";
  }

  StateImpl::UpdateOptions AccountStatementsState::create_update_options() const {
    StateImpl::UpdateOptions result{};
    
    // Add '-' key option for back navigation
    result.add('-', {"Back", []() -> StateUpdateResult {
      using StringMsg = CargoMsgT<std::string>;
      return {std::nullopt, []() -> std::optional<Msg> {
        return std::make_shared<PopStateMsg>(
          std::make_shared<StringMsg>("AccountStatementsState")
        );
      }};
    }});
    
    return result;
  }

}