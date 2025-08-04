#pragma once

#include "StateImpl.hpp"
#include "fiscal/amount/TaggedAmountFramework.hpp"
#include "FiscalPeriod.hpp"
#include "PeriodConstrainedContent.hpp"
#include "Mod10View.hpp"

namespace first {
  class TaggedAmountsState : public StateImpl {
  public:
    using TaggedAmount = ::TaggedAmount;
    using TaggedAmounts = ::TaggedAmounts;
    using TaggedAmountsSlice = PeriodSlice<TaggedAmounts>;

    TaggedAmountsState(TaggedAmountsSlice const& tagged_amounts_slice, Mod10View mod10_view);
    TaggedAmountsState(TaggedAmountsSlice const& tagged_amounts_slice);
    TaggedAmountsState(TaggedAmountsState const&) = delete; // Force construct from data = ux ok

    virtual StateUpdateResult update(Msg const& msg) const override;

    virtual StateImpl::UpdateOptions create_update_options() const override;
    virtual std::string caption() const override;
    virtual StateImpl::UX create_ux() const override;

  private:
    PeriodSlice<TaggedAmounts> m_tagged_amounts_slice;
    first::Mod10View m_mod10_view;
    
    StateUpdateResult apply(cargo::EditedItem<TaggedAmount> const& edited_tagged_amount) const;
  }; // struct TaggedAmountsState
}