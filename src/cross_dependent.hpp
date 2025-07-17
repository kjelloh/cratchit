#pragma once

#include <memory> // std::shared_ptr
#include <functional> // std::function

// The Elm Architecture
namespace TEA {
  template <class MaybeState, class MaybeNullCmd>
  struct UpdateResultT {
    MaybeState maybe_state;
    MaybeNullCmd maybe_null_cmd;
    operator bool() const {return maybe_state.has_value() or (maybe_null_cmd != nullptr);}
    void apply(MaybeState::value_type& target_state_ref,MaybeNullCmd& target_cmd_ref) {
      if (maybe_state) {        
        if (*maybe_state == nullptr) {
          throw(std::runtime_error("UpdateResultT::apply: DESIGN_INSUFFICIENCY - maybe_state carries nullptr (breaks designed behaviour)"));
        }
        target_state_ref = *maybe_state;
      }
      if (maybe_null_cmd) target_cmd_ref = maybe_null_cmd;
    }
  };

  // Helper pair<..> to struct<...>{}
  template <class MaybeState, class MaybeNullCmd>
  UpdateResultT<MaybeState,MaybeNullCmd> make_update_result(std::pair<MaybeState,MaybeNullCmd> pp) {
    return {
        .maybe_state = pp.first
      ,.maybe_null_cmd = pp.second
    };
  }

  // Helper separate MaybeState,Cmd -> UpdateResultT
  template <class MaybeState, class MaybeNullCmd>
  UpdateResultT<MaybeState,MaybeNullCmd> make_update_result(MaybeState const& maybe_state,MaybeNullCmd const& maybe_null_cmd) {
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
  using MaybeState = std::optional<State>; // for future non-ptr State...

  // ----------------------------------
  // Helper template to create states uniformly
  template<typename StateType, typename... Args>
  std::shared_ptr<StateType> make_state(Args&&... args) {
    return std::make_shared<StateType>(std::forward<Args>(args)...);
  }

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
  using MaybeNullCmd = Cmd; // Helper alias to make Cmd type 'clear'

  // Declared here, defined in cmd unit
  extern std::optional<Msg> Nop();

  // cratchit TEA helper aliases
  using StateUpdateResult = TEA::UpdateResultT<MaybeState,MaybeNullCmd>;

} // namespace first

