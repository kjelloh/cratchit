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

    // TODO: Refactor key procesing into this method, step-by-step
    //       When done, move into update above (and remove this refactoring step)
    virtual std::optional<Cmd> cmd_from_key(char key) const override;

  };
} // namespace first