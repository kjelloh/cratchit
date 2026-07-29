#include "update.hpp"
#include "ViewState.hpp"
#include "msg_to_string.hpp"
#include "log.hpp"

// Helpers to visit State with a Msg (Double dispatch)
namespace detail {

  template<typename S, typename M>
  concept Updateable = requires(S s, M m) {
    { s.update(m) } -> std::same_as<Transition<ViewState>>;
  };

  template<typename S, typename M>
  auto update(S const& s, M const& m) {
      if constexpr (Updateable<S, M>) {
        // Call State::operator(Msg)
        return s.update((m));
      }
      else {
        return Transition<ViewState>{TransitionKind::Ignore, s};   // ignore unsupported messages
      }
  }
} // detail

auto double_dispatch_update_to_transition(ViewState const& state, const tea::Msg& msg) {
  // 1. Dispatch to concrete ViewState
  // 2. Dispatch to concrete Msg
  return std::visit(
    // state on captured msg
    [&msg](auto const& s) {
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

namespace tea {

  Model update(Model const& model,Msg const& msg) {

    if (!is_no_msg(msg)) {
      log_development_trace(
        "update for msg:{}"
        ,msg_to_string(msg)
      );
    }

    auto apply_view_state_transition = [](
         Model::ViewStateStack const& view_state_stack
        ,Transition<ViewState> const& transition) -> Model::ViewStateStack {
      switch (transition.kind()) {
        case TransitionKind::Mutate:
          return view_state_stack.set(
            view_state_stack.size()-1
            ,transition.next_state()
          );
        case TransitionKind::Ignore:
          return view_state_stack;
        default:
          log_design_insufficiency(
            "apply_view_state_transition: unhandled transition kind:{}"
            ,static_cast<int>(transition.kind())
          );
          return view_state_stack;
      }
    }; // apply_view_state_transition

    if (model.view_state_stack().size() > 0) {
      auto view_state_transition = double_dispatch_update_to_transition(model.view_state_stack().back(),msg);
      auto next_view_state_stack = apply_view_state_transition(model.view_state_stack(),view_state_transition);
      return model.with_mutated_view_state_stack(next_view_state_stack);
    }

    return model; // fallback

  } // update
} // tea
