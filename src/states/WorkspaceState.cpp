#include "WorkspaceState.hpp"
#include <spdlog/spdlog.h> 

namespace first {
  // ----------------------------------
  WorkspaceState::WorkspaceState(StateImpl::UX ux) : StateImpl{ux} {
    this->add_option('0',{"ITfied AB",itfied_factory});        
    this->add_option('1',{"Org x",orx_x_factory});        
  }

  // ----------------------------------
  WorkspaceState::~WorkspaceState() {
    spdlog::info("WorkspaceState destructor executed");
  }
}