#include "May2AprilState.hpp"

namespace first {
  May2AprilState::May2AprilState(StateImpl::UX ux) : StateImpl{ux} {
    this->add_option('0',{"RBD:s",RBDs_factory});
  }
}
