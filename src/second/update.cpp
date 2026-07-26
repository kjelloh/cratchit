#include "update.hpp"
#include "State.hpp"

namespace tea {

  // Helpers to visit State with a Msg (Double dispatch)
  namespace detail {
    template<typename S, typename M>
    concept Applicable = requires(S const& s, M const& m) {
      // s provides call operator on m that returns a State ok
      { s(m) } -> std::same_as<State>;
    };

    template<typename S, typename M>
    State apply(S& s, M const& m) {
        if constexpr (Applicable<S, M>) {
          // Call State::operator(Msg)
          return s(m);
        }
        else {
          return s;   // ignore unsupported messages
        }
    }
  } // detail

  State double_dispatch(State const& state, const tea::Msg& msg) {
    // 1. Dispatch to concrete State
    // 2. Dispatch to concrete Msg
    // = apply concrete msg to concrete state with fallback if state has no handler for msg
    return std::visit(
      // state on captured msg
      [&msg](auto const& s) -> State {
        return std::visit(
          // captured message on captured state
          [&s](auto const& m) {
            // apply state on msg
            return detail::apply(s, m);
          }
          ,msg
        );        
      }
      ,state
    );
  }


  // helper type for the visitor
  template<class... Ts>
  struct overloaded : Ts... { using Ts::operator()...; };

  Model update(Model const& model,Msg const& msg) {
  
    if (model.state_stack().size() > 0) {
      // Test double dispatch
      auto s = double_dispatch(model.state_stack().back(),msg);
    }

    return std::visit(overloaded{
        [&model](NoMsg no_key_msg) {
          return model;
        }
        ,[&model](UnicodeKeyMsg unicode_key_msg) {
          return model.with_pushed_unicode(unicode_key_msg.code_point);
        }
        ,[&model](BackspaceKeyMsg const& backspace_msg) {
          return model.with_popped_unicode();
        }
      }
      ,msg
    );
  } // update
} // tea
