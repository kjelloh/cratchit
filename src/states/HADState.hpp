#pragma once

#include "StateImpl.hpp"
#include "fiscal/amount/HADFramework.hpp"
#include <format>

namespace first {
  // ----------------------------------
  struct HADState : public StateImpl {
    StateFactory SIE_factory = []() {
      auto SIE_ux = StateImpl::UX{"HAD to SIE UX goes here"};
      return std::make_shared<StateImpl>(SIE_ux);
    };
    using HAD = HeadingAmountDateTransEntry;
    HAD m_had;
    HADState(HAD had);

    static StateFactory factory_from(HADState::HAD const& had);
    static StateImpl::Option option_from(HADState::HAD const& had);
  };
} // namespace first