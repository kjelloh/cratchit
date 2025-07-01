#pragma once

#include "StateImpl.hpp"
#include "fiscal/amount/HADFramework.hpp"
#include <format>

namespace first {
  // ----------------------------------
  struct HADState : public StateImpl {
    using HAD = HeadingAmountDateTransEntry;
    HAD m_had;
    HADState(HAD had);

    static StateFactory factory_from(HADState::HAD const& had);
    static StateImpl::Option option_from(HADState::HAD const& had);

  };
} // namespace first