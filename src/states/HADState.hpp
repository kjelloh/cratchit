#pragma once

#include "StateImpl.hpp"
#include "fiscal/amount/HADFramework.hpp"
#include <format>

namespace first {
  // ----------------------------------
  struct HADState : public StateImpl {
    using HAD = HeadingAmountDateTransEntry;
    cargo::EditedItem<HAD> m_edited_had;
    HADState(HAD had);

    virtual StateUpdateResult update(Msg const& msg) const override;
    virtual StateImpl::UpdateOptions create_update_options() const override;
    virtual UX create_ux() const override;
  };
} // namespace first