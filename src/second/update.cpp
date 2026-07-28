#include "update.hpp"
#include "ViewState.hpp"
#include "msg_to_string.hpp"
#include "log.hpp"

namespace tea {

  Model update(Model const& model,Msg const& msg) {

    if (!is_no_msg(msg)) {
      log_development_trace(
        "update for msg:{}"
        ,msg_to_string(msg)
      );
    }

    auto apply_transition = [](
         Model::AppStateStack const& app_state_stack
        ,Transition<AppState> const& transition) -> Model::AppStateStack {
      switch (transition.kind()) {
        case TransitionKind::Mutate:
          return app_state_stack.set(
            app_state_stack.size()-1
            ,transition.next_state()
          );
        case TransitionKind::Ignore:
          return app_state_stack;
        default:
          log_design_insufficiency(
            "apply_transition: unhandled transition kind:{}"
            ,static_cast<int>(transition.kind())
          );
          return app_state_stack;
      }
    }; // apply_transition

    if (model.app_state_stack().size() > 0) {
      auto transition = model.app_state_stack().back().update(msg);
      auto next_app_state_stack = apply_transition(model.app_state_stack(),transition);
      return model.with_mutated_stack(next_app_state_stack);  
    }

    return model; // fallback

  } // update
} // tea
