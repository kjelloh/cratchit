#include "VATReturnsState.hpp"
#include <format>

namespace first {
  // ----------------------------------
  VATReturnsState::VATReturnsState(StateImpl::UX ux) : StateImpl{ux} {}

  StateFactory VATReturnsState::factory_from() {
    // Called by parent state so all_hads will exist as long as this callable is avaibale (option in parent state)
    return []() {
      auto ux = StateImpl::UX{"VAT Returns UX goes here"};
      return std::make_shared<VATReturnsState>(ux);
    };
  }

  // StateImpl::CmdOption VATReturnsState::cmd_option_from() {
  //   return {std::format("VAT Returns"), cmd_from_state_factory(factory_from())};
  // }

  StateImpl::UpdateOptions VATReturnsState::create_update_options() const {
    StateImpl::UpdateOptions result{};
    // TODO: Refactor add_update_option in constructor to update options here
    // TODO: Refactor add_cmd_option in constructor to update options here
    return result;
  }

}
