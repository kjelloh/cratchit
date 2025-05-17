#pragma once

#include "StateImpl.hpp"
#include "ProjectState.hpp"

namespace first {
  // ----------------------------------
  struct WorkspaceState : public StateImpl {
    StateFactory itfied_factory = []() {
      auto itfied_ux = StateImpl::UX{"ITfied UX"};
      return std::make_shared<ProjectState>(itfied_ux);
    };

    StateFactory orx_x_factory = []() {
      auto org_x_ux = StateImpl::UX{"Other Organisation UX"};
      return std::make_shared<ProjectState>(org_x_ux);
    };

    WorkspaceState(StateImpl::UX ux);
    ~WorkspaceState();
  }; // Workspace StateImpl
}