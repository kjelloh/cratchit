#pragma once

#include "model.hpp"
#include "cross_dependent.hpp"
#include "cmd.hpp"
#include "tea/runtime.hpp"

namespace first {

  // TEA Runtime class that inherits from the template
  class CratchitRuntime : public TEA::Runtime<Model,Msg,Cmd> {
  public:
    // Inherit constructors
    using TEA::Runtime<Model,Msg,Cmd>::Runtime;
  };

  // TEA Runtime type aliases
  using InitResult = CratchitRuntime::init_result;
  using ModelUpdateResult = CratchitRuntime::model_update_result;
  using ViewResult = CratchitRuntime::view_result;

  // TEA functions - exposed for testing
  bool is_quit_msg(Msg const& msg);
  InitResult init();
  ModelUpdateResult update(Model model, Msg msg);
  ViewResult view(const Model &model);

} // namespace first