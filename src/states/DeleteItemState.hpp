#pragma once

#include "StateImpl.hpp"
#include <format>

namespace first {
  // ----------------------------------
  template <typename T>
  struct DeleteItemState : public StateImpl {
    using Item = T;
    cargo::EditedItem<Item> m_edited_item;

    DeleteItemState(Item item)
      : m_edited_item{item, cargo::ItemMutation::UNCHANGED}, StateImpl({}) {
      ux().clear();
      ux().push_back(std::format("Delete: {} ?", to_string(item)));

      this->add_update_option('y', {"Yes", [this]() -> UpdateResult {
        auto new_state = std::make_shared<DeleteItemState<Item>>(*this);
        new_state->m_edited_item.mutation = cargo::ItemMutation::DELETED;
        return {new_state, []() -> std::optional<Msg> { 
          return std::make_shared<PopStateMsg>(); 
        }};
      }});
      this->add_cmd_option('n', {"No", Nop});
    }

    virtual Cargo get_cargo() const override {
      return cargo::to_cargo(m_edited_item);
    }

    static StateFactory factory_from(DeleteItemState::Item const& item) {
      return [item]() {
        return std::make_shared<DeleteItemState<Item>>(item);
      };
    }
  };
} // namespace first