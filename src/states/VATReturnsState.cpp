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

  // StateFactory VATReturnsState::factory_from() {
  //   // Called by parent state so all_hads will exist as long as this callable is avaibale (option in parent state)
  //   return []() {
  //     return make_state<VATReturnsState>("VAT Returns");
  //   };
  // }

  // StateImpl::CmdOption VATReturnsState::cmd_option_from() {
  //   return {std::format("VAT Returns"), cmd_from_state_factory(factory_from())};
  // }

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
