#pragma once

#include "StateImpl.hpp"

namespace first {
  // ----------------------------------
  class ProjectState : public StateImpl {
  public:
    ProjectState(StateImpl::UX ux);
  };
}