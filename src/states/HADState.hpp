#pragma once

#include "StateImpl.hpp"
#include "fiscal/amount/HADFramework.hpp"
#include "cargo/EditedItemCargo.hpp"
#include <format>

namespace first {
  // ----------------------------------
  struct HADState : public StateImpl {
    using HAD = HeadingAmountDateTransEntry;
    cargo::EditedItem<HAD> m_edited_had;
    HADState(HAD had);

    // virtual StateUpdateResult apply(cargo::EditedItemCargo<HAD> const& cargo) const override;
    virtual StateUpdateResult update(Msg const& msg) const override;

    // virtual Cargo get_cargo() const override;
    virtual std::optional<Msg> get_on_destruct_msg() const override;

    void update_ux();

    static StateFactory factory_from(HADState::HAD const& had);
    static StateImpl::CmdOption cmd_option_from(HADState::HAD const& had);

  };
} // namespace first