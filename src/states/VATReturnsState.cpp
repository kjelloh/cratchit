#include "VATReturnsState.hpp"
#include <format>

namespace first {
  // ----------------------------------
  VATReturnsState::VATReturnsState() : StateImpl{} {}

  std::string VATReturnsState::caption() const {
    if (m_caption.has_value()) {
      return m_caption.value();
    }
    return "VAT Returns";
  }


  StateImpl::UpdateOptions VATReturnsState::create_update_options() const {
    StateImpl::UpdateOptions result{};
    // TODO: Refactor add_update_option in constructor to update options here
    // TODO: Refactor add_cmd_option in constructor to update options here
    
    // Add '-' key option last - back with empty string as payload (VATReturnsState has no significant data)
    result.add('-', {"Back", []() -> StateUpdateResult {
      using StringMsg = CargoMsgT<std::string>;
      return {std::nullopt, []() -> std::optional<Msg> {
        return std::make_shared<PopStateMsg>(
          std::make_shared<StringMsg>("VATReturnsState")
        );
      }};
    }});
    
    return result;
  }

}
