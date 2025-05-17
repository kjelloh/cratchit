#include "ProjectState.hpp"

namespace first {

  // ----------------------------------
  ProjectState::ProjectState(StateImpl::UX ux) : StateImpl{ux} {
    this->add_option('0',{"May to April",may2april_factory});
    this->add_option('1',{"Q1",q1_factory});
  }
}