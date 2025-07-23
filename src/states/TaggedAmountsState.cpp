#include "TaggedAmountsState.hpp"
#include <spdlog/spdlog.h>

namespace first {
  
  TaggedAmountsState::TaggedAmountsState(TaggedAmounts all_tagged_amounts, FiscalPeriod fiscal_period, Mod10View mod10_view)
    : m_all_tagged_amounts{all_tagged_amounts}
    , m_fiscal_period{fiscal_period}
    , m_mod10_view{mod10_view}
    , StateImpl() {

    spdlog::info("TaggedAmountsState::TaggedAmountsState(TaggedAmounts all_tagged_amounts:size={},Mod10View mod10_view)", m_all_tagged_amounts.size());
  }

  TaggedAmountsState::TaggedAmountsState(TaggedAmounts all_tagged_amounts, FiscalPeriod fiscal_period) 
    : TaggedAmountsState(all_tagged_amounts, fiscal_period, Mod10View(all_tagged_amounts)) {}

  StateImpl::UX TaggedAmountsState::create_ux() const {
    UX result{};
    result.push_back(std::format("{}", m_fiscal_period.to_string()));
    for (size_t i : m_mod10_view) {
      auto entry = std::to_string(i);
      entry += ". ";
      entry += to_string(m_all_tagged_amounts[i]);
      result.push_back(entry);
    }
    return result;
  }

  StateUpdateResult TaggedAmountsState::update(Msg const& msg) const {
    spdlog::info("TaggedAmountsState::update - not yet implemented");
    return {};
  }

  StateImpl::UpdateOptions TaggedAmountsState::create_update_options() const {
    StateImpl::UpdateOptions result{};
    
    // Add '-' key option for back navigation
    result.add('-', {"Back", [this]() -> StateUpdateResult {
      using TaggedAmountsMsg = CargoMsgT<TaggedAmountsState::TaggedAmounts>;
      return {std::nullopt, [all_tagged_amounts = this->m_all_tagged_amounts]() -> std::optional<Msg> {
        return std::make_shared<PopStateMsg>(
          std::make_shared<TaggedAmountsMsg>(all_tagged_amounts)
        );
      }};
    }});
    
    return result;
  }

} // namespace first