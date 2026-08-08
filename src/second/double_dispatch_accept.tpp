#pragma once

namespace detail {

  template<typename ConcreteState>
  concept ProvidesDataStateUpdate = requires(ConcreteState concrete_state) {
    { concrete_state.update(std::declval<DataState const&>) } -> std::same_as<DataState>;
  };

  template<typename ConcreteState>
  auto update(ConcreteState const& concrete_state,DataState const& data_state) {
    if constexpr (ProvidesDataStateUpdate<ConcreteState>) {
      return concrete_state.update(data_state);
    }
    else {
      return data_state; // fallback
    }
  }

  template<typename ConcreteState>
  concept ProvidesViewStateAccept = requires(ConcreteState concrete_state) {
    { concrete_state.accept(std::declval<ViewState const&>())} -> std::same_as<ConcreteState>;
  };

  template<typename ConcreteState>
  ViewState accept(ConcreteState const& concrete_target,ViewState const& source) {
    if constexpr (ProvidesViewStateAccept<ConcreteState>) {
      return concrete_target.accept(source);
    }
    return concrete_target; // Fallback
  }

} // detail

ViewState double_dispatch_accept(ViewState const& target, ViewState const& source) {
  // 1. Dispatch to target accept
  return std::visit(
    [&source](auto const& concrete_target){
      return detail::accept(concrete_target,source);
    }
    ,target
  );
  return target;
} // accept
