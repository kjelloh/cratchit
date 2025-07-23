#pragma once

#include "StateImpl.hpp"
#include "fiscal/amount/TaggedAmountFramework.hpp"
#include "FiscalPeriod.hpp"
#include "Mod10View.hpp"

namespace first {
  class TaggedAmountsState : public StateImpl {
  public:
    using TaggedAmount = ::TaggedAmount;
    using TaggedAmounts = ::TaggedAmounts;

    TaggedAmountsState(TaggedAmounts all_tagged_amounts, FiscalPeriod fiscal_period, Mod10View mod10_view);
    TaggedAmountsState(TaggedAmounts all_tagged_amounts, FiscalPeriod fiscal_period);
    TaggedAmountsState(TaggedAmountsState const&) = delete; // Force construct from data = ux ok

    virtual StateUpdateResult update(Msg const& msg) const override;

    virtual StateImpl::UpdateOptions create_update_options() const override;
    virtual StateImpl::UX create_ux() const override;

  private:
    TaggedAmounts m_all_tagged_amounts;
    FiscalPeriod m_fiscal_period;
    first::Mod10View m_mod10_view;
    
    StateUpdateResult apply(cargo::EditedItem<TaggedAmount> const& edited_tagged_amount) const;
  }; // struct TaggedAmountsState
}