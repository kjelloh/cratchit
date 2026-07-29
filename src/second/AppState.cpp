#include "AppState.hpp"

// Helpers to visit State with a Msg (Double dispatch)
namespace detail {

  template<typename S, typename M>
  concept Updateable = requires(S s, M m) {
    { s.update(m) } -> std::same_as<S>;
  };

  template<typename S, typename M>
  ViewState update(S const& s, M const& m) {
      if constexpr (Updateable<S, M>) {
        // Call State::operator(Msg)
        return s.update((m));
      }
      else {
        return s;   // ignore unsupported messages
      }
  }
} // detail

ViewState double_dispatch_update(ViewState const& state, const tea::Msg& msg) {
  // 1. Dispatch to concrete ViewState
  // 2. Dispatch to concrete Msg
  // = update concrete view state for concrete msg with fallback if state has no handler for msg
  return std::visit(
    // state on captured msg
    [&msg](auto const& s) -> ViewState {
      return std::visit(
        // captured message on captured state
        [&s](auto const& m) {
          // update state on msg
          return detail::update(s, m);
        }
        ,msg
      );        
    }
    ,state
  );
}

AppState AppState::with_view_state(ViewState view_state) const {
  AppState result{*this};
  result.m_view_state = view_state;
  return result;
}

Transition<AppState> AppState::update(tea::Msg const& msg) const {
  auto next_view = double_dispatch_update(m_view_state,msg);
  return Transition<AppState>(TransitionKind::Mutate,this->with_view_state(next_view));
}

tea::Ux AppState::view() const {
  return std::visit(
    [](auto const& s) -> tea::Ux {
      return s.view();
    }
    ,m_view_state
  );
}
