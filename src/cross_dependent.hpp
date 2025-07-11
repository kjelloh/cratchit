#pragma once

#include <memory> // std::shared_ptr
#include <functional> // std::function

// The Elm Architecture
namespace TEA {
  template <class S, class MaybeNullC>
  struct UpdateResultT {
    std::optional<S> maybe_state;
    MaybeNullC maybe_null_cmd;
    operator bool() const {return maybe_state.has_value() or (maybe_null_cmd != nullptr);}
    void apply(S& target_state_ref,MaybeNullC& target_cmd_ref) {
      if (maybe_state) target_state_ref = *maybe_state;
      if (maybe_null_cmd) target_cmd_ref = maybe_null_cmd;
    }
  };

  // Helper pair<..> to struct<...>{}
  template <class S, class MaybeNullC>
  UpdateResultT<S,MaybeNullC> make_update_result(std::pair<std::optional<S>,MaybeNullC> pp) {
    return {
        .maybe_state = pp.first
      ,.maybe_null_cmd = pp.second
    };
  }

  // Helper separate MaybeState,Cmd -> UpdateResultT
  template <class S, class MaybeNullC>
  UpdateResultT<S,MaybeNullC> make_update_result(std::optional<S> const& maybe_state,MaybeNullC const& maybe_null_cmd) {
    return {
       .maybe_state = maybe_state
      ,.maybe_null_cmd = maybe_null_cmd
    };
  }


} // TEA

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

  // using StateUpdateResult = std::pair<std::optional<State>, Cmd>;
  using StateUpdateResult = TEA::UpdateResultT<State,Cmd>;

} // namespace first

