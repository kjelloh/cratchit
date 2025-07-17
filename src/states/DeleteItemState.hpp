#pragma once

#include "StateImpl.hpp"
#include "msgs/msg.hpp"
#include <format>
#include "spdlog/spdlog.h"

namespace first {
  // ----------------------------------
  template <typename T>
  struct DeleteItemState : public StateImpl {
    using Item = T;
    using EditedItem = cargo::EditedItem<Item>;
    EditedItem m_edited_item;

    DeleteItemState(Item item)
      : m_edited_item{item, cargo::ItemMutation::UNCHANGED}, StateImpl() {
      // UX creation moved to create_ux()
      // ux().clear();
      // ux().push_back(std::format("Delete: {} ?", to_string(item)));

      // this->add_update_option('y', {"Yes", [this]() -> StateUpdateResult {
      //   using EditedHADMsg = CargoMsgT<cargo::EditedItem<HAD>>;
      //   std::shared_ptr<DeleteItemState<Item>> new_state = to_cloned(*this, this->m_edited_item.item);
      //   new_state->m_edited_item.mutation = cargo::ItemMutation::DELETED;
      //   spdlog::info("DeleteItemState::m_update_options['y'] captures payload {}",to_string(new_state->m_edited_item.item));
      //   return {new_state, [payload = new_state->m_edited_item]() -> std::optional<Msg> { 
      //     spdlog::info("DeleteItemState::m_update_options['y'] lambda forwards payload {}",to_string(payload.item));
      //     return std::make_shared<PopStateMsg>(
      //       std::make_shared<EditedHADMsg>(payload)
      //     ); 
      //   }};
      // }});
      // this->add_cmd_option('n', {"No", Nop});
    }

    virtual StateImpl::UpdateOptions create_update_options() const override {
      StateImpl::UpdateOptions result{};
      using EditedHADMsg = CargoMsgT<cargo::EditedItem<HAD>>;

      // TODO: Refactor add_update_option in constructor to update options here

      result.add('y', {"Yes", [this]() -> StateUpdateResult {
        auto new_state = make_state<DeleteItemState<Item>>(this->m_edited_item.item);
        new_state->m_edited_item.mutation = cargo::ItemMutation::DELETED;
        spdlog::info("DeleteItemState::m_update_options['y'] captures payload {}",to_string(new_state->m_edited_item.item));
        return {new_state, [payload = new_state->m_edited_item]() -> std::optional<Msg> { 
          spdlog::info("DeleteItemState::m_update_options['y'] lambda forwards payload {}",to_string(payload.item));
          return std::make_shared<PopStateMsg>(
            std::make_shared<EditedHADMsg>(payload)
          ); 
        }};
      }});

      // TODO: Refactor add_cmd_option in constructor to update options here

      result.add('n',{"No",[payload = this->m_edited_item]() -> StateUpdateResult {
        return {std::nullopt,[payload]() -> std::optional<Msg>{
          return std::make_shared<PopStateMsg>(
            std::make_shared<EditedHADMsg>(payload)
          ); 
        }};
      }});

      // Add '-' key option last - back with current state as cargo
      result.add('-', {"Back", [payload = this->m_edited_item]() -> StateUpdateResult {
        return {std::nullopt, [payload]() -> std::optional<Msg> {
          return std::make_shared<PopStateMsg>(
            std::make_shared<EditedHADMsg>(payload)
          );
        }};
      }});

      return result;
    }

    virtual StateImpl::UX create_ux() const override {
      UX result{};
      result.push_back(std::format("Delete: {} ?", to_string(m_edited_item.item)));
      return result;
    }

    // virtual Cargo get_cargo() const override {
    //   return cargo::to_cargo(m_edited_item);
    // }
    // virtual std::optional<Msg> get_on_destruct_msg() const override {
    //   return std::make_shared<CargoMsgT<cargo::EditedItem<Item>>>(m_edited_item);
    // }

    static StateFactory factory_from(DeleteItemState::Item const& item) {
      return [item]() {
        return std::make_shared<DeleteItemState<Item>>(item);
      };
    }
  };
} // namespace first