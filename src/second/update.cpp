#include "update.hpp"
#include "ViewState.hpp"
#include "msg_to_string.hpp"
#include "log.hpp"

// Helpers to visit State with a Msg (Double dispatch)
namespace detail {

  template<typename S, typename M>
  concept Updateable = requires(S s, M m) {
    { s.update(m) } -> std::same_as<std::tuple<
       Transition<ViewState>
      ,Cmd>
    >;
  };

  template<typename S, typename M>
  auto update(S const& s, M const& m) {
      if constexpr (Updateable<S, M>) {
        // Call State::operator(Msg)
        return s.update((m));
      }
      else {
        return std::make_tuple(
          Transition<ViewState>{TransitionKind::Ignore, s}
          ,Cmd{}
        );
      }
  } // update
} // detail

auto double_dispatch_view_update(ViewState const& state, const tea::Msg& msg) {
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

  std::tuple<Model,Cmd> update(Model const& model,Msg const& msg) {

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
        case TransitionKind::Push:
          return view_state_stack.push_back(transition.next_state());
        case TransitionKind::Mutate:
          return view_state_stack.set(
            view_state_stack.size()-1
            ,transition.next_state()
          );
        case TransitionKind::Ignore:
          return view_state_stack;
        case TransitionKind::Accept: {
          auto accepted = view_state_stack.back();
          auto mutated_stack = view_state_stack.take(view_state_stack.size()-1); // pop
          mutated_stack = mutated_stack.set(
            mutated_stack.size()-1
            ,double_dispatch_accept(mutated_stack.back(),accepted)
          );
          return mutated_stack;
        }
        case TransitionKind::Reject: {
          auto mutated_stack = view_state_stack.take(view_state_stack.size()-1); // pop
          return mutated_stack;
        }
        default:
          log_design_insufficiency(
            "apply_view_state_transition: unhandled transition kind:{}"
            ,static_cast<int>(transition.kind())
          );
          return view_state_stack;
      }
    }; // apply_view_state_transition

    if (model.view_state_stack().size() > 0) {
      auto [view_state_transition,cmd] = double_dispatch_view_update(model.view_state_stack().back(),msg);
      auto next_view_state_stack = apply_view_state_transition(model.view_state_stack(),view_state_transition);
      return {
        model.with_mutated_view_state_stack(next_view_state_stack)
        ,cmd
      };
    }

    return {
        model
        ,Cmd{}
    }; // fallback

  } // update
} // tea
