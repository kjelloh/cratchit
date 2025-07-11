#pragma once

#include <memory> // std::shared_ptr
#include <functional> // std::function

namespace first {
  // ----------------------------------
  struct StateImpl; // Forward

  // ----------------------------------
  using State = std::shared_ptr<StateImpl>;

  // ----------------------------------
  using StateFactory = std::function<State()>;
  
  // ----------------------------------
  struct MsgImpl {
    virtual ~MsgImpl() = default;
  };

  // ----------------------------------
  using Msg = std::shared_ptr<MsgImpl>;

  // Cmd: () -> std::optional<Msg>
  // ----------------------------------
  using Cmd = std::function<std::optional<Msg>()>;

  // Declared here, defined in cmd unit
  extern std::optional<Msg> Nop();

} // namespace first

namespace first {

  namespace tea {

    template <class S>
    struct UpdateResultT {
      std::optional<S> maybe_state;
      Cmd maybe_null_cmd;
      operator bool() const {return maybe_state.has_value() or (maybe_null_cmd != nullptr);}
      void apply(S& target_state,Cmd& target_cmd) {
        if (maybe_state) target_state = *maybe_state;
        if (maybe_null_cmd) target_cmd = maybe_null_cmd;
      }
    };

    template <class S>
    UpdateResultT<S> make_update_result(std::pair<std::optional<S>,Cmd> pp) {
      return {
         .maybe_state = pp.first
        ,.maybe_null_cmd = pp.second
      };
    }
  }

  using StateUpdateResult = std::pair<std::optional<State>, Cmd>;

}
