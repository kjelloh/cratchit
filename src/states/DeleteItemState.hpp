#pragma once

#include "StateImpl.hpp"
#include <format>

namespace first {
  // ----------------------------------
  template <typename T>
  struct DeleteItemState : public StateImpl {
    using Item = T;
    Item m_item;

    DeleteItemState(Item item)
      : m_item{item}, StateImpl({}) {
      ux().clear();
      ux().push_back(std::format("Delete: {} ?", to_string(item)));

      this->add_cmd_option('y', {"Yes", Nop});
      this->add_cmd_option('n', {"No", Nop});
    }

    static StateFactory factory_from(DeleteItemState::Item const& item) {
      return [item]() {
        return std::make_shared<DeleteItemState<Item>>(item);
      };
    }
    static StateImpl::Option option_from(DeleteItemState::Item const& item);
  };
} // namespace first