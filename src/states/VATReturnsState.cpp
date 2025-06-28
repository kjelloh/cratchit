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

  StateImpl::Option VATReturnsState::option_from() {
    return {std::format("VAT Returns"), factory_from()};
  }

}
