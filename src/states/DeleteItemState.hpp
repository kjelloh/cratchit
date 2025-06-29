#pragma once

#include "StateImpl.hpp"
#include <format>

namespace first {
  // ----------------------------------
  template <typename T>
  struct DeleteItemState : public StateImpl {
    StateFactory SIE_factory = []() {
      auto SIE_ux = StateImpl::UX{"Item to SIE UX goes here"};
      return std::make_shared<StateImpl>(SIE_ux);
    };
    using Item = T;
    Item m_item;
    DeleteItemState(Item item)
      : m_item{item}, StateImpl({}) {
      ux().clear();
      ux().push_back(std::format("Delete {} ?", to_string(item)));
    }

    static StateFactory factory_from(DeleteItemState::Item const& item);
    static StateImpl::Option option_from(DeleteItemState::Item const& item);
  };
} // namespace first