#include "update.hpp"
#include "State.hpp"
#include "msg_to_string.hpp"
#include "log.hpp"

namespace tea {

  // Helpers to visit State with a Msg (Double dispatch)
  namespace detail {
    template<typename S, typename M>
    concept Updateable = requires(S s, M m) {
      { s.update(m) } -> std::same_as<S>;
    };

    template<typename S, typename M>
    State update(S const& s, M const& m) {
        if constexpr (Updateable<S, M>) {
          // Call State::operator(Msg)
          return s.update((m));
        }
        else {
          return s;   // ignore unsupported messages
        }
    }
  } // detail

  State double_dispatch_update(State const& state, const tea::Msg& msg) {
    // 1. Dispatch to concrete State
    // 2. Dispatch to concrete Msg
    // = update concrete msg to concrete state with fallback if state has no handler for msg
    return std::visit(
      // state on captured msg
      [&msg](auto const& s) -> State {
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

  Model update(Model const& model,Msg const& msg) {

    if (!is_no_msg(msg)) {
      log_development_trace(
        "update for msg:{}"
        ,msg_to_string(msg)
      );
    }

    auto apply_transition = [](Model::AppStateStack const& app_state_stack, Transition<AppState> const& transition) -> Model::AppStateStack {
      switch (transition.kind()) {
        case TransitionKind::Ignore:
          return app_state_stack;
        default:
          return app_state_stack;
      }
    }; // apply_transition

    if (model.app_state_stack().size() > 0) {
      auto transition = model.app_state_stack().back().update(msg);
      auto next_app_state_stack = apply_transition(model.app_state_stack(),transition);
      return model.with_mutated_stack(next_app_state_stack);  
    }
  
    if (model.state_stack().size() > 0) {
      // auto next = model.with_mutated_state(double_dispatch_update(model.state_stack().back(),msg));
      return model.with_mutated_state(double_dispatch_update(model.state_stack().back(),msg));
    }

    return model;

  } // update
} // tea
