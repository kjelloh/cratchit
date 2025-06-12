#pragma once

#include "StateImpl.hpp"
#include "VATReturnsState.hpp"

namespace first {
  struct Q1State : public StateImpl {
    StateFactory VATReturns_factory = []() {
      auto VATReturns_ux = StateImpl::UX{"VAT Returns UX goes here"};
      return std::make_shared<VATReturnsState>(VATReturns_ux);
    };
    Q1State(StateImpl::UX ux);
  };
}