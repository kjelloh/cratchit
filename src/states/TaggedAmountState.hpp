#pragma once

#include "StateImpl.hpp"
#include "fiscal/amount/TaggedAmountFramework.hpp"
#include "cargo/CargoBase.hpp"
#include <format>

namespace first {
  // ----------------------------------
  struct TaggedAmountState : public StateImpl {
    using TaggedAmount = ::TaggedAmount;
    cargo::EditedItem<TaggedAmount> m_edited_tagged_amount;
    TaggedAmountState(TaggedAmount tagged_amount);

    virtual StateUpdateResult update(Msg const& msg) const override;
    virtual StateImpl::UpdateOptions create_update_options() const override;
    virtual UX create_ux() const override;
  };
} // namespace first