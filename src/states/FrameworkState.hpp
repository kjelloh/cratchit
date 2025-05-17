#pragma once

#include "StateImpl.hpp"
#include "WorkspaceState.hpp"

namespace first {
  // ----------------------------------
  struct FrameworkState : public StateImpl {
    StateFactory workspace_0_factory = []() {
      auto workspace_0_ux = StateImpl::UX{"Workspace UX"};
      return std::make_shared<WorkspaceState>(workspace_0_ux);
    };

    FrameworkState(StateImpl::UX ux);
    ~FrameworkState();
    virtual std::pair<std::optional<State>, Cmd> update(Msg const &msg);
  }; // struct FrameworkState
}