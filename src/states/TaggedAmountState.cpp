#include "TaggedAmountState.hpp"
#include "DeleteItemState.hpp"
#include "msgs/msg.hpp"
#include "spdlog/spdlog.h"

namespace first {
// ----------------------------------
  TaggedAmountState::TaggedAmountState(TaggedAmount tagged_amount) 
    : m_edited_tagged_amount{tagged_amount, cargo::ItemMutation::UNCHANGED}, StateImpl() {
  }

  StateUpdateResult TaggedAmountState::update(Msg const& msg) const {
    using EditedTaggedAmountMsg = CargoMsgT<cargo::EditedItem<TaggedAmount>>;
    if (auto pimpl = std::dynamic_pointer_cast<EditedTaggedAmountMsg>(msg); pimpl != nullptr) {
      auto new_state = make_state<TaggedAmountState>(pimpl->payload.item);
      new_state->m_edited_tagged_amount = pimpl->payload;
      spdlog::info("TaggedAmountState::update - Received EditedTaggedAmountMsg with payload {}, new_state->m_edited_tagged_amount {} "
        ,to_string(pimpl->payload.item)
        ,to_string(new_state->m_edited_tagged_amount.item));
      return {new_state, Nop};
    }
    return {};
  }

  StateImpl::UX TaggedAmountState::create_ux() const {
    UX result{};
    std::string status_prefix;
    switch (m_edited_tagged_amount.mutation) {
      case cargo::ItemMutation::UNCHANGED:
        status_prefix = "";
        break;
      case cargo::ItemMutation::MODIFIED:
        status_prefix = "[MODIFIED] ";
        break;
      case cargo::ItemMutation::DELETED:
        status_prefix = "[MARKED FOR DELETION] ";
        break;
    }
    
    result.push_back(status_prefix + to_string(m_edited_tagged_amount.item));

    return result;
  }

  StateImpl::UpdateOptions TaggedAmountState::create_update_options() const {
    StateImpl::UpdateOptions result{};

    // Add export option (placeholder for now, similar to HAD -> SIE)
    StateFactory export_factory = []() {
      return make_state<StateImpl>("TaggedAmount Export"); // Placeholder state
    };

    result.add('0',{"Export",[export_factory]() -> StateUpdateResult {
      return {std::nullopt,[new_state = export_factory()]() -> std::optional<Msg> {
        return std::make_shared<PushStateMsg>(new_state);
      }};
    }});

    // Add delete option
    result.add('d',{"Delete",[tagged_amount = this->m_edited_tagged_amount.item]() -> StateUpdateResult {
      return {std::nullopt,[tagged_amount]() -> std::optional<Msg> {
        State new_state = make_state<DeleteItemState<TaggedAmount>>(tagged_amount);
        auto msg = std::make_shared<PushStateMsg>(new_state);
        return msg;
      }};
    }});
    
    // Add OK option to return to parent with changes
    result.add('o', {"OK", [this]() -> StateUpdateResult {
      spdlog::info("TaggedAmountState 'o' lambda: capturing m_edited_tagged_amount: {}", to_string(this->m_edited_tagged_amount.item));
      using EditedTaggedAmountMsg = CargoMsgT<cargo::EditedItem<TaggedAmount>>;
      return {std::nullopt, [payload = this->m_edited_tagged_amount]() -> std::optional<Msg> {
        spdlog::info("TaggedAmountState 'o' lambda execution: payload: {}", to_string(payload.item));
        return std::make_shared<PopStateMsg>(
          std::make_shared<EditedTaggedAmountMsg>(payload)
        );
      }};
    }});

    return result;
  }

} // namespace first