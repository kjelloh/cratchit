#include "TaggedAmountsState.hpp"
#include "TaggedAmountState.hpp"
#include "logger/log.hpp"
#include <algorithm>
#include <ranges>

namespace first {
  
  TaggedAmountsState::TaggedAmountsState(TaggedAmountsSlice const& tagged_amounts_slice, Mod10View mod10_view)
    : StateImpl()
    , m_tagged_amounts_slice{tagged_amounts_slice}
    , m_mod10_view{mod10_view} {

    spdlog::info("TaggedAmountsState::TaggedAmountsState(TaggedAmountsSlice const& tagged_amounts_slice::size={},Mod10View mod10_view)", tagged_amounts_slice.content().size());
  }

  TaggedAmountsState::TaggedAmountsState(TaggedAmountsSlice const& tagged_amounts_slice)
    : TaggedAmountsState(tagged_amounts_slice, Mod10View(tagged_amounts_slice.content())) {}

  std::string TaggedAmountsState::caption() const {
    if (m_tagged_amounts_slice.content().size() > 0) {
      return std::format("Tagged Amounts:{}",m_tagged_amounts_slice.content().size());
    }
    return std::format("Tagged Amounts:{}"," *Empty*");
  }

  StateImpl::UX TaggedAmountsState::create_ux() const {
    UX result{};
    result.push_back(this->caption());
    result.push_back(std::format("{}", m_tagged_amounts_slice.period().to_string()));
    for (size_t i : m_mod10_view) {
      auto entry = std::to_string(i);
      entry += ". ";
      entry += to_string(m_tagged_amounts_slice.content()[i]);
      result.push_back(entry);
    }
    return result;
  }

  StateUpdateResult TaggedAmountsState::update(Msg const& msg) const {
    using EditedTaggedAmountMsg = CargoMsgT<cargo::EditedItem<TaggedAmount>>;
    using TaggedAmountsMsg = CargoMsgT<TaggedAmountsState::TaggedAmounts>;
    
    if (auto entry_msg_ptr = std::dynamic_pointer_cast<UserEntryMsg>(msg); entry_msg_ptr != nullptr) {
      spdlog::info("TaggedAmountsState::update - handling UserEntryMsg");
      std::string command(entry_msg_ptr->m_entry);
      // TODO: Implement TaggedAmount parsing from user input
      spdlog::info("TaggedAmountsState::update - TaggedAmount parsing not yet implemented");
    }
    else if (auto pimpl = std::dynamic_pointer_cast<TaggedAmountsMsg>(msg); pimpl != nullptr) {
      spdlog::info("TaggedAmountsState::update - handling TaggedAmountsMsg");
      if (this->m_tagged_amounts_slice.content() != pimpl->payload) {
        spdlog::info("TaggedAmountsState::update - TaggedAmounts has changed");
        auto mutated_tagged_amounts = pimpl->payload;
        // Sort by date for consistent ordering
        std::sort(mutated_tagged_amounts.begin(), mutated_tagged_amounts.end(), 
                  [](const TaggedAmount& a, const TaggedAmount& b) {
                    return a.date() < b.date();
                  });
        auto mutated_state = make_state<TaggedAmountsState>(TaggedAmountsSlice{this->m_tagged_amounts_slice.period(), mutated_tagged_amounts});
        return {mutated_state, Cmd{}};
      }
    }
    else if (auto pimpl = std::dynamic_pointer_cast<EditedTaggedAmountMsg>(msg); pimpl != nullptr) {
      spdlog::info("TaggedAmountsState::update - handling EditedTaggedAmountMsg");
      return apply(pimpl->payload);
    }
    return {};
  }

  StateImpl::UpdateOptions TaggedAmountsState::create_update_options() const {
    StateImpl::UpdateOptions result{};
    
    // Use enhanced API for cleaner digit-based option creation
    for (char digit = '0'; digit <= '9'; ++digit) {
      if (!m_mod10_view.is_valid_digit(digit)) {
        break; // No more valid digits
      }
      
      // Try to get direct index for single items
      if (auto index = m_mod10_view.direct_index(digit); index.has_value()) {
        // Single TaggedAmount - direct navigation to TaggedAmountState
        auto caption = to_string(m_tagged_amounts_slice.content()[*index]);
        result.add(digit, {caption, [tagged_amount = m_tagged_amounts_slice.content()[*index]]() -> StateUpdateResult {
          return {std::nullopt, [tagged_amount]() -> std::optional<Msg> {
            State new_state = make_state<TaggedAmountState>(tagged_amount);
            return std::make_shared<PushStateMsg>(new_state);
          }};
        }});
      } else {
        // Subrange - use drill_down for navigation
        try {
          auto drilled_view = m_mod10_view.drill_down(digit);
          auto caption = std::format("{} .. {}", drilled_view.first(), drilled_view.second() - 1);
          result.add(digit, {caption, [tagged_amounts_slice = m_tagged_amounts_slice, drilled_view]() -> StateUpdateResult {
            return {std::nullopt, [tagged_amounts_slice, drilled_view]() -> std::optional<Msg> {
              State new_state = make_state<TaggedAmountsState>(tagged_amounts_slice, drilled_view);
              return std::make_shared<PushStateMsg>(new_state);
            }};
          }});
        } catch (const std::exception& e) {
          spdlog::error("TaggedAmountsState: Error creating option for digit '{}': {}", digit, e.what());
        }
      }
    }
    
    // Add '-' key option last - back with TaggedAmounts as payload
    result.add('-', {"Back", [this]() -> StateUpdateResult {
      using TaggedAmountsMsg = CargoMsgT<TaggedAmountsState::TaggedAmounts>;
      return {std::nullopt, [all_tagged_amounts = this->m_tagged_amounts_slice.content()]() -> std::optional<Msg> {
        return std::make_shared<PopStateMsg>(
          std::make_shared<TaggedAmountsMsg>(all_tagged_amounts)
        );
      }};
    }});
    
    return result;
  }

  StateUpdateResult TaggedAmountsState::apply(cargo::EditedItem<TaggedAmount> const& edited_tagged_amount) const {
    auto mutated_tagged_amounts = this->m_tagged_amounts_slice.content();
    MaybeState maybe_state{}; // default 'none'
    
    switch (edited_tagged_amount.mutation) {
      case cargo::ItemMutation::DELETED:
        // Remove the TaggedAmount from the collection
        if (auto iter = std::ranges::find(mutated_tagged_amounts, edited_tagged_amount.item); iter != mutated_tagged_amounts.end()) {
          mutated_tagged_amounts.erase(iter);
          spdlog::info("TaggedAmountsState::apply - TaggedAmount {} DELETED ok", to_string(*iter));
          maybe_state = make_state<TaggedAmountsState>(TaggedAmountsSlice{this->m_tagged_amounts_slice.period(), mutated_tagged_amounts});
        }
        else {
          spdlog::error("TaggedAmountsState::apply - failed to find TaggedAmount {} to delete", to_string(edited_tagged_amount.item));
        }
        break;
        
      case cargo::ItemMutation::MODIFIED:
        // Future: replace existing TaggedAmount with modified version
        spdlog::info("TaggedAmountsState::apply - TaggedAmount {} marked as MODIFIED (not yet implemented)", to_string(edited_tagged_amount.item));
        break;
        
      case cargo::ItemMutation::UNCHANGED:
        spdlog::info("TaggedAmountsState::apply - TaggedAmount {} returned unchanged", to_string(edited_tagged_amount.item));
        // No state change needed
        break;
    }
    
    return {maybe_state, Cmd{}};
  }

} // namespace first